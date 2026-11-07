#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <ast/ast.h>
#include <ast/symbols.h>
#include <frontend/bison/bison_parser.h>
#include <support/hash_map.h>
#include <support/mem.h>
#include <support/util.h>

#include "builtins.h"
#include "support/str.h"
#include "support/types.h"

static dyn_arena *builtins_arena;
static dyn_arena *eval_arena;

static Type *_attr_string_type;
static Type *_attr_bool_type;
static Type *_attr_map_type;

/* -- Element data ---------------------------------------------------------- */

typedef enum {
  ACCEPT,
  ACCEPT_CHARSET,
  ACCESSKEY,
  ACTION,
  ALLOW,
  ALPHA,
  ALT,
  ARIA,
  AS,
  ASYNC,
  AUTOCAPITALIZE,
  AUTOCOMPLETE,
  AUTOCORRECT,
  AUTOFOCUS,
  AUTOPLAY,
  CAPTURE,
  CHARSET,
  CHECKED,
  CITE,
  CLASS,
  COLORSPACE,
  COLS,
  COLSPAN,
  CONTENT,
  CONTENTEDITABLE,
  CONTROLS,
  COORDS,
  CROSSORIGIN,
  CSP,
  DATA,
  DATASET,
  DATETIME,
  DECODING,
  DEFAULT_ATTR,
  DEFER,
  DIR,
  DIRNAME,
  DISABLED,
  DOWNLOAD,
  DRAGGABLE,
  ENCTYPE,
  ENTERKEYHINT,
  ELEMENTTIMING,
  FETCHPRIORITY,
  HTML_FOR,
  FORM,
  FORMACTION,
  FORMENCTYPE,
  FORMMETHOD,
  FORMNOVALIDATE,
  FORMTARGET,
  HEADERS,
  HEIGHT,
  HIDDEN,
  HIGH,
  HREF,
  HREFLANG,
  HTTP_EQUIV,
  ID,
  INERT,
  INTEGRITY,
  INPUTMODE,
  ISMAP,
  ITEMID,
  ITEMPROP,
  ITEMREF,
  ITEMSCOPE,
  ITEMTYPE,
  KIND,
  LABEL,
  LANG,
  LOADING,
  LIST,
  LOOP,
  LOW,
  MAX,
  MAXLENGTH,
  MEDIA,
  METHOD,
  MIN,
  MINLENGTH,
  MULTIPLE,
  MUTED,
  NAME,
  NOMODULE,
  NONCE,
  NOVALIDATE,
  OPEN,
  OPTIMUM,
  PATTERN,
  PART,
  PING,
  PLACEHOLDER,
  PLAYSINLINE,
  POSTER,
  POPOVER,
  PRELOAD,
  READONLY,
  REFERRERPOLICY,
  REL,
  REQUIRED,
  REVERSED,
  ROLE,
  ROWS,
  ROWSPAN,
  SANDBOX,
  SCOPE,
  SELECTED,
  SHAPE,
  SIZE,
  SIZES,
  SLOT,
  SPAN,
  SPELLCHECK,
  SRC,
  SRCDOC,
  SRCLANG,
  SRCSET,
  START,
  STEP,
  STYLE,
  TABINDEX,
  TARGET,
  TITLE,
  TRANSLATE,
  TYPE,
  USEMAP,
  VALUE,
  WIDTH,
  WRAP,
  HTML_ATTR_COUNT,
} HTMLAttributeName;

typedef enum {
  EL_HTML,
  EL_BASE,
  EL_HEAD,
  EL_LINK,
  EL_META,
  EL_STYLE,
  EL_TITLE,
  EL_BODY,
  EL_ADDRESS,
  EL_ARTICLE,
  EL_ASIDE,
  EL_FOOTER,
  EL_HEADER,
  EL_H1,
  EL_H2,
  EL_H3,
  EL_H4,
  EL_H5,
  EL_H6,
  EL_HGROUP,
  EL_MAIN,
  EL_NAV,
  EL_SECTION,
  EL_SEARCH,
  EL_BLOCKQUOTE,
  EL_DD,
  EL_DIV,
  EL_DL,
  EL_DT,
  EL_FIGCAPTION,
  EL_FIGURE,
  EL_HR,
  EL_LI,
  EL_MENU,
  EL_OL,
  EL_P,
  EL_PRE,
  EL_UL,
  EL_A,
  EL_ABBR,
  EL_B,
  EL_BDI,
  EL_BDO,
  EL_BR,
  EL_CITE,
  EL_CODE,
  EL_DATA,
  EL_DFN,
  EL_EM,
  EL_I,
  EL_KBD,
  EL_MARK,
  EL_Q,
  EL_RP,
  EL_RT,
  EL_RUBY,
  EL_S,
  EL_SAMP,
  EL_SMALL,
  EL_SPAN,
  EL_STRONG,
  EL_SUB,
  EL_SUP,
  EL_TIME,
  EL_U,
  EL_VAR,
  EL_WBR,
  EL_AREA,
  EL_AUDIO,
  EL_IMG,
  EL_MAP,
  EL_TRACK,
  EL_VIDEO,
  EL_EMBED,
  EL_IFRAME,
  EL_OBJECT,
  EL_PICTURE,
  EL_SOURCE,
  EL_SVG,
  EL_MATH,
  EL_CANVAS,
  EL_NOSCRIPT,
  EL_SCRIPT,
  EL_DEL,
  EL_INS,
  EL_CAPTION,
  EL_COL,
  EL_COLGROUP,
  EL_TABLE,
  EL_TBODY,
  EL_TD,
  EL_TFOOT,
  EL_TH,
  EL_THEAD,
  EL_TR,
  EL_BUTTON,
  EL_DATALIST,
  EL_FIELDSET,
  EL_FORM,
  EL_INPUT,
  EL_LABEL,
  EL_LEGEND,
  EL_METER,
  EL_OPTGROUP,
  EL_OPTION,
  EL_OUTPUT,
  EL_PROGRESS,
  EL_SELECT,
  EL_SELECTEDCONTENT,
  EL_TEXTAREA,
  EL_DETAILS,
  EL_DIALOG,
  EL_GEOLOCATION,
  EL_SUMMARY,
  EL_SLOT,
  EL_TEMPLATE,
  HTML_ELEM_COUNT,
} HTMLElementName;

typedef struct {
  const char *tag;
  bool self_closing;
} HTMLElement;

typedef struct {
  const char *name;
  BuiltinType type;
} HTMLAttribute;

// Oh you're a web developer? Name every HTML
static HTMLElement _elements[] = {
  /* Main root */
  [EL_HTML]             = { "html",             false },
  /* Document Metadata */
  [EL_BASE]             = { "base",             true  },
  [EL_HEAD]             = { "head",             false },
  [EL_LINK]             = { "link",             true  },
  [EL_META]             = { "meta",             true  },
  [EL_STYLE]            = { "style",            false },
  [EL_TITLE]            = { "title",            false },
  /* Sectioning root */
  [EL_BODY]             = { "body",             false },
  /* Content sectioning */
  [EL_ADDRESS]          = { "address",          false },
  [EL_ARTICLE]          = { "article",          false },
  [EL_ASIDE]            = { "aside",            false },
  [EL_FOOTER]           = { "footer",           false },
  [EL_HEADER]           = { "header",           false },
  [EL_H1]               = { "h1",               false },
  [EL_H2]               = { "h2",               false },
  [EL_H3]               = { "h3",               false },
  [EL_H4]               = { "h4",               false },
  [EL_H5]               = { "h5",               false },
  [EL_H6]               = { "h6",               false },
  [EL_HGROUP]           = { "hgroup",           false },
  [EL_MAIN]             = { "main",             false },
  [EL_NAV]              = { "nav",              false },
  [EL_SECTION]          = { "section",          false },
  [EL_SEARCH]           = { "search",           false },
  /* Text content */
  [EL_BLOCKQUOTE]       = { "blockquote",       false },
  [EL_DD]               = { "dd",               false },
  [EL_DIV]              = { "div",              false },
  [EL_DL]               = { "dl",               false },
  [EL_DT]               = { "dt",               false },
  [EL_FIGCAPTION]       = { "figcaption",       false },
  [EL_FIGURE]           = { "figure",           false },
  [EL_HR]               = { "hr",               true  },
  [EL_LI]               = { "li",               false },
  [EL_MENU]             = { "menu",             false },
  [EL_OL]               = { "ol",               false },
  [EL_P]                = { "p",                false },
  [EL_PRE]              = { "pre",              false },
  [EL_UL]               = { "ul",               false },
  /* Inline text semantics */
  [EL_A]                = { "a",                false },
  [EL_ABBR]             = { "abbr",             false },
  [EL_B]                = { "b",                false },
  [EL_BDI]              = { "bdi",              false },
  [EL_BDO]              = { "bdo",              false },
  [EL_BR]               = { "br",               true  },
  [EL_CITE]             = { "cite",             false },
  [EL_CODE]             = { "code",             false },
  [EL_DATA]             = { "data",             false },
  [EL_DFN]              = { "dfn",              false },
  [EL_EM]               = { "em",               false },
  [EL_I]                = { "i",                false },
  [EL_KBD]              = { "kbd",              false },
  [EL_MARK]             = { "mark",             false },
  [EL_Q]                = { "q",                false },
  [EL_RP]               = { "rp",               false },
  [EL_RT]               = { "rt",               false },
  [EL_RUBY]             = { "ruby",             false },
  [EL_S]                = { "s",                false },
  [EL_SAMP]             = { "samp",             false },
  [EL_SMALL]            = { "small",            false },
  [EL_SPAN]             = { "span",             false },
  [EL_STRONG]           = { "strong",           false },
  [EL_SUB]              = { "sub",              false },
  [EL_SUP]              = { "sup",              false },
  [EL_TIME]             = { "time",             false },
  [EL_U]                = { "u",                false },
  [EL_VAR]              = { "var",              false },
  [EL_WBR]              = { "wbr",              true  },
  /* Image and multimedia */
  [EL_AREA]             = { "area",             true  },
  [EL_AUDIO]            = { "audio",            false },
  [EL_IMG]              = { "img",              true  },
  [EL_MAP]              = { "map",              false },
  [EL_TRACK]            = { "track",            true  },
  [EL_VIDEO]            = { "video",            false },
  /* Embedded content */
  [EL_EMBED]            = { "embed",            true  },
  [EL_IFRAME]           = { "iframe",           false },
  [EL_OBJECT]           = { "object",           false },
  [EL_PICTURE]          = { "picture",          false },
  [EL_SOURCE]           = { "source",           true  },
  /* SVG and MathML */
  [EL_SVG]              = { "svg",              false },
  [EL_MATH]             = { "math",             false },
  /* Scripting */
  [EL_CANVAS]           = { "canvas",           false },
  [EL_NOSCRIPT]         = { "noscript",         false },
  [EL_SCRIPT]           = { "script",           false },
  /* Demarcating edits */
  [EL_DEL]              = { "del",              false },
  [EL_INS]              = { "ins",              false },
  /* Table content */
  [EL_CAPTION]          = { "caption",          false },
  [EL_COL]              = { "col",              false },
  [EL_COLGROUP]         = { "colgroup",         false },
  [EL_TABLE]            = { "table",            false },
  [EL_TBODY]            = { "tbody",            false },
  [EL_TD]               = { "td",               false },
  [EL_TFOOT]            = { "tfoot",            false },
  [EL_TH]               = { "th",               false },
  [EL_THEAD]            = { "thead",            false },
  [EL_TR]               = { "tr",               false },
  /* Forms */
  [EL_BUTTON]           = { "button",           false },
  [EL_DATALIST]         = { "datalist",         false },
  [EL_FIELDSET]         = { "fieldset",         false },
  [EL_FORM]             = { "form",             false },
  [EL_INPUT]            = { "input",            false },
  [EL_LABEL]            = { "label",            false },
  [EL_LEGEND]           = { "legend",           false },
  [EL_METER]            = { "meter",            false },
  [EL_OPTGROUP]         = { "optgroup",         false },
  [EL_OPTION]           = { "option",           false },
  [EL_OUTPUT]           = { "output",           false },
  [EL_PROGRESS]         = { "progress",         false },
  [EL_SELECT]           = { "select",           false },
  [EL_SELECTEDCONTENT]  = { "selectedcontent",  false },
  [EL_TEXTAREA]         = { "textarea",         false },
  /* Interactive elements */
  [EL_DETAILS]          = { "details",          false },
  [EL_DIALOG]           = { "dialog",           false },
  [EL_GEOLOCATION]      = { "geolocation",      false },
  [EL_SUMMARY]          = { "summary",          false },
  /* Web Components (who uses these???) */
  [EL_SLOT]             = { "slot",             false },
  [EL_TEMPLATE]         = { "template",         false },
};

static HTMLAttribute _attributes[] = {
  [ACCEPT]          = { "accept",           T_STRING },
  [ACCEPT_CHARSET]  = { "accept-charset",   T_STRING },
  [ACCESSKEY]       = { "accesskey",        T_STRING },
  [ACTION]          = { "action",           T_STRING },
  [ALLOW]           = { "allow",            T_STRING },
  [ALPHA]           = { "alpha",            T_BOOL   },
  [ALT]             = { "alt",              T_STRING },
  [ARIA]            = { "aria",             T_VOID   },
  [AS]              = { "as",               T_STRING },
  [ASYNC]           = { "async",            T_BOOL   },
  [AUTOCAPITALIZE]  = { "autocapitalize",   T_STRING },
  [AUTOCOMPLETE]    = { "autocomplete",     T_STRING },
  [AUTOCORRECT]     = { "autocorrect",      T_STRING },
  [AUTOFOCUS]       = { "autofocus",        T_BOOL   },
  [AUTOPLAY]        = { "autoplay",         T_BOOL   },
  [CAPTURE]         = { "capture",          T_STRING },
  [CHARSET]         = { "charset",          T_STRING },
  [CHECKED]         = { "checked",          T_BOOL   },
  [CITE]            = { "cite",             T_STRING },
  [CLASS]           = { "class",            T_STRING },
  [COLORSPACE]      = { "colorspace",       T_STRING },
  [COLS]            = { "cols",             T_STRING },
  [COLSPAN]         = { "colspan",          T_STRING },
  [CONTENT]         = { "content",          T_STRING },
  [CONTENTEDITABLE] = { "contenteditable",  T_STRING },
  [CONTROLS]        = { "controls",         T_BOOL   },
  [COORDS]          = { "coords",           T_STRING },
  [CROSSORIGIN]     = { "crossorigin",      T_STRING },
  [CSP]             = { "csp",              T_STRING },
  [DATA]            = { "data",             T_STRING },
  [DATASET]         = { "dataset",          T_VOID   },
  [DATETIME]        = { "datetime",         T_STRING },
  [DECODING]        = { "decoding",         T_STRING },
  [DEFAULT_ATTR]    = { "default",          T_BOOL   },
  [DEFER]           = { "defer",            T_BOOL   },
  [DIR]             = { "dir",              T_STRING },
  [DIRNAME]         = { "dirname",          T_STRING },
  [DISABLED]        = { "disabled",         T_BOOL   },
  [DOWNLOAD]        = { "download",         T_STRING },
  [DRAGGABLE]       = { "draggable",        T_STRING },
  [ENCTYPE]         = { "enctype",          T_STRING },
  [ENTERKEYHINT]    = { "enterkeyhint",     T_STRING },
  [ELEMENTTIMING]   = { "elementtiming",    T_STRING },
  [FETCHPRIORITY]   = { "fetchpriority",    T_STRING },
  [HTML_FOR]        = { "html_for",         T_STRING },
  [FORM]            = { "form",             T_STRING },
  [FORMACTION]      = { "formaction",       T_STRING },
  [FORMENCTYPE]     = { "formenctype",      T_STRING },
  [FORMMETHOD]      = { "formmethod",       T_STRING },
  [FORMNOVALIDATE]  = { "formnovalidate",   T_BOOL   },
  [FORMTARGET]      = { "formtarget",       T_STRING },
  [HEADERS]         = { "headers",          T_STRING },
  [HEIGHT]          = { "height",           T_STRING },
  [HIDDEN]          = { "hidden",           T_STRING },
  [HIGH]            = { "high",             T_STRING },
  [HREF]            = { "href",             T_STRING },
  [HREFLANG]        = { "hreflang",         T_STRING },
  [HTTP_EQUIV]      = { "http-equiv",       T_STRING },
  [ID]              = { "id",               T_STRING },
  [INERT]           = { "inert",            T_BOOL   },
  [INTEGRITY]       = { "integrity",        T_STRING },
  [INPUTMODE]       = { "inputmode",        T_STRING },
  [ISMAP]           = { "ismap",            T_BOOL   },
  [ITEMID]          = { "itemid",           T_STRING },
  [ITEMPROP]        = { "itemprop",         T_STRING },
  [ITEMREF]         = { "itemref",          T_STRING },
  [ITEMSCOPE]       = { "itemscope",        T_BOOL   },
  [ITEMTYPE]        = { "itemtype",         T_STRING },
  [KIND]            = { "kind",             T_STRING },
  [LABEL]           = { "label",            T_STRING },
  [LANG]            = { "lang",             T_STRING },
  [LOADING]         = { "loading",          T_STRING },
  [LIST]            = { "list",             T_STRING },
  [LOOP]            = { "loop",             T_BOOL   },
  [LOW]             = { "low",              T_STRING },
  [MAX]             = { "max",              T_STRING },
  [MAXLENGTH]       = { "maxlength",        T_STRING },
  [MEDIA]           = { "media",            T_STRING },
  [METHOD]          = { "method",           T_STRING },
  [MIN]             = { "min",              T_STRING },
  [MINLENGTH]       = { "minlength",        T_STRING },
  [MULTIPLE]        = { "multiple",         T_BOOL   },
  [MUTED]           = { "muted",            T_BOOL   },
  [NAME]            = { "name",             T_STRING },
  [NOMODULE]        = { "nomodule",         T_BOOL   },
  [NONCE]           = { "nonce",            T_STRING },
  [NOVALIDATE]      = { "novalidate",       T_BOOL   },
  [OPEN]            = { "open",             T_BOOL   },
  [OPTIMUM]         = { "optimum",          T_STRING },
  [PATTERN]         = { "pattern",          T_STRING },
  [PART]            = { "part",             T_STRING },
  [PING]            = { "ping",             T_STRING },
  [PLACEHOLDER]     = { "placeholder",      T_STRING },
  [PLAYSINLINE]     = { "playsinline",      T_BOOL   },
  [POSTER]          = { "poster",           T_STRING },
  [POPOVER]         = { "popover",          T_STRING },
  [PRELOAD]         = { "preload",          T_STRING },
  [READONLY]        = { "readonly",         T_BOOL   },
  [REFERRERPOLICY]  = { "referrerpolicy",   T_STRING },
  [REL]             = { "rel",              T_STRING },
  [REQUIRED]        = { "required",         T_BOOL   },
  [REVERSED]        = { "reversed",         T_BOOL   },
  [ROLE]            = { "role",             T_STRING },
  [ROWS]            = { "rows",             T_STRING },
  [ROWSPAN]         = { "rowspan",          T_STRING },
  [SANDBOX]         = { "sandbox",          T_STRING },
  [SCOPE]           = { "scope",            T_STRING },
  [SELECTED]        = { "selected",         T_BOOL   },
  [SHAPE]           = { "shape",            T_STRING },
  [SIZE]            = { "size",             T_STRING },
  [SIZES]           = { "sizes",            T_STRING },
  [SLOT]            = { "slot",             T_STRING },
  [SPAN]            = { "span",             T_STRING },
  [SPELLCHECK]      = { "spellcheck",       T_STRING },
  [SRC]             = { "src",              T_STRING },
  [SRCDOC]          = { "srcdoc",           T_STRING },
  [SRCLANG]         = { "srclang",          T_STRING },
  [SRCSET]          = { "srcset",           T_STRING },
  [START]           = { "start",            T_STRING },
  [STEP]            = { "step",             T_STRING },
  [STYLE]           = { "style",            T_STRING },
  [TABINDEX]        = { "tabindex",         T_STRING },
  [TARGET]          = { "target",           T_STRING },
  [TITLE]           = { "title",            T_STRING },
  [TRANSLATE]       = { "translate",        T_STRING },
  [TYPE]            = { "type",             T_STRING },
  [USEMAP]          = { "usemap",           T_STRING },
  [VALUE]           = { "value",            T_STRING },
  [WIDTH]           = { "width",            T_STRING },
  [WRAP]            = { "wrap",             T_STRING },
};

static HTMLAttributeName _global_attrs[] = {
  ACCESSKEY, AUTOCAPITALIZE, AUTOCORRECT, AUTOFOCUS, CLASS, CONTENTEDITABLE,
  DATASET, DIR, DRAGGABLE, ENTERKEYHINT, HIDDEN, ID, INERT, INPUTMODE, ITEMID,
  ITEMPROP, ITEMREF, ITEMSCOPE, ITEMTYPE, LANG, NONCE, PART, POPOVER, ROLE,
  SLOT, SPELLCHECK, STYLE, TABINDEX, TITLE, TRANSLATE,
};

static HTMLAttributeName _a_attrs[] = { DOWNLOAD, HREF, HREFLANG, MEDIA, PING, REFERRERPOLICY, REL, SHAPE, TARGET, -1 };
static HTMLAttributeName _area_attrs[] = { ALT, COORDS, DOWNLOAD, HREF, MEDIA, PING, REFERRERPOLICY, REL, SHAPE, TARGET, -1 };
static HTMLAttributeName _audio_attrs[] = { AUTOPLAY, CONTROLS, CROSSORIGIN, LOOP, MUTED, PRELOAD, SRC, -1 };
static HTMLAttributeName _base_attrs[] = { HREF, TARGET, -1 };
static HTMLAttributeName _blockquote_q_attrs[] = { CITE, -1 };
static HTMLAttributeName _button_attrs[] = { DISABLED, FORM, FORMACTION, FORMENCTYPE, FORMMETHOD, FORMNOVALIDATE, FORMTARGET, NAME, TYPE, VALUE, -1 };
static HTMLAttributeName _canvas_attrs[] = { HEIGHT, WIDTH, -1 };
static HTMLAttributeName _col_colgroup_attrs[] = { SPAN, -1 };
static HTMLAttributeName _data_li_attrs[] = { VALUE, -1 };
static HTMLAttributeName _del_ins_attrs[] = { CITE, DATETIME, -1 };
static HTMLAttributeName _details_dialog_attrs[] = { OPEN, -1 };
static HTMLAttributeName _embed_attrs[] = { HEIGHT, SRC, TYPE, WIDTH, -1 };
static HTMLAttributeName _fieldset_attrs[] = { DISABLED, FORM, NAME, -1 };
static HTMLAttributeName _form_attrs[] = { ACCEPT, ACCEPT_CHARSET, ACTION, AUTOCOMPLETE, ENCTYPE, METHOD, NAME, NOVALIDATE, TARGET, -1 };
static HTMLAttributeName _iframe_attrs[] = { ALLOW, CSP, HEIGHT, LOADING, NAME, REFERRERPOLICY, SANDBOX, SRC, SRCDOC, WIDTH, -1 };
static HTMLAttributeName _img_attrs[] = { ALT, CROSSORIGIN, DECODING, ELEMENTTIMING, FETCHPRIORITY, HEIGHT, ISMAP, LOADING, REFERRERPOLICY, SIZES, SRC, SRCSET, USEMAP, WIDTH, -1 };
static HTMLAttributeName _input_attrs[] = { ACCEPT, ALPHA, ALT, AUTOCOMPLETE, CAPTURE, CHECKED, COLORSPACE, DIRNAME, DISABLED, FORM, FORMACTION, FORMENCTYPE, FORMMETHOD, FORMNOVALIDATE, FORMTARGET, HEIGHT, LIST, MAX, MAXLENGTH, MIN, MINLENGTH, MULTIPLE, NAME, PATTERN, PLACEHOLDER, READONLY, REQUIRED, SIZE, SRC, STEP, TYPE, USEMAP, VALUE, WIDTH, -1 };
static HTMLAttributeName _label_attrs[] = { HTML_FOR, -1 };
static HTMLAttributeName _link_attrs[] = { AS, CROSSORIGIN, FETCHPRIORITY, HREF, HREFLANG, INTEGRITY, MEDIA, REFERRERPOLICY, REL, SIZES, TYPE, -1 };
static HTMLAttributeName _map_attrs[] = { NAME, -1 };
static HTMLAttributeName _menu_attrs[] = { TYPE, -1 };
static HTMLAttributeName _meta_attrs[] = { CHARSET, CONTENT, HTTP_EQUIV, NAME, -1 };
static HTMLAttributeName _meter_attrs[] = { HIGH, LOW, MAX, MIN, OPTIMUM, VALUE, -1 };
static HTMLAttributeName _object_attrs[] = { DATA, FORM, HEIGHT, NAME, TYPE, USEMAP, WIDTH, -1 };
static HTMLAttributeName _ol_attrs[] = { REVERSED, START, TYPE, -1 };
static HTMLAttributeName _optgroup_attrs[] = { DISABLED, LABEL, -1 };
static HTMLAttributeName _option_attrs[] = { DISABLED, LABEL, SELECTED, VALUE, -1 };
static HTMLAttributeName _output_attrs[] = { HTML_FOR, FORM, NAME, -1 };
static HTMLAttributeName _p_attrs[] = { ELEMENTTIMING, -1 };
static HTMLAttributeName _progress_attrs[] = { MAX, VALUE, -1 };
static HTMLAttributeName _script_attrs[] = { ASYNC, CROSSORIGIN, DEFER, FETCHPRIORITY, INTEGRITY, REFERRERPOLICY, SRC, TYPE, -1 };
static HTMLAttributeName _select_attrs[] = { AUTOCOMPLETE, DISABLED, FORM, MULTIPLE, NAME, REQUIRED, SIZE, -1 };
static HTMLAttributeName _source_attrs[] = { MEDIA, SIZES, SRC, SRCSET, TYPE, -1 };
static HTMLAttributeName _style_attrs[] = { MEDIA, TYPE, -1 };
static HTMLAttributeName _td_attrs[] = { COLSPAN, HEADERS, ROWSPAN, -1 };
static HTMLAttributeName _textarea_attrs[] = { AUTOCOMPLETE, COLS, DIRNAME, DISABLED, ENTERKEYHINT, FORM, INPUTMODE, MAXLENGTH, MINLENGTH, NAME, PLACEHOLDER, READONLY, REQUIRED, ROWS, WRAP, -1 };
static HTMLAttributeName _th_attrs[] = { COLSPAN, HEADERS, ROWSPAN, SCOPE, -1 };
static HTMLAttributeName _time_attrs[] = { DATETIME, -1 };
static HTMLAttributeName _track_attrs[] = { DEFAULT_ATTR, KIND, LABEL, SRC, SRCLANG, -1 };
static HTMLAttributeName _video_attrs[] = { AUTOPLAY, CONTROLS, CROSSORIGIN, ELEMENTTIMING, HEIGHT, LOOP, MUTED, PLAYSINLINE, POSTER, PRELOAD, SRC, WIDTH, -1 };

static HTMLAttributeName *_attrs_per_element[HTML_ELEM_COUNT] = {
  [EL_A] = _a_attrs,
  [EL_AREA] = _area_attrs,
  [EL_AUDIO] = _audio_attrs,
  [EL_BASE] = _base_attrs,
  [EL_BLOCKQUOTE] = _blockquote_q_attrs,
  [EL_Q] = _blockquote_q_attrs,
  [EL_BUTTON] = _button_attrs,
  [EL_CANVAS] = _canvas_attrs,
  [EL_COL] = _col_colgroup_attrs,
  [EL_COLGROUP] = _col_colgroup_attrs,
  [EL_DATA] = _data_li_attrs,
  [EL_LI] = _data_li_attrs,
  [EL_DEL] = _del_ins_attrs,
  [EL_INS] = _del_ins_attrs,
  [EL_DETAILS] = _details_dialog_attrs,
  [EL_DIALOG] = _details_dialog_attrs,
  [EL_EMBED] = _embed_attrs,
  [EL_FIELDSET] = _fieldset_attrs,
  [EL_FORM] = _form_attrs,
  [EL_IFRAME] = _iframe_attrs,
  [EL_IMG] = _img_attrs,
  [EL_INPUT] = _input_attrs,
  [EL_LABEL] = _label_attrs,
  [EL_LINK] = _link_attrs,
  [EL_MAP] = _map_attrs,
  [EL_MENU] = _menu_attrs,
  [EL_META] = _meta_attrs,
  [EL_METER] = _meter_attrs,
  [EL_OBJECT] = _object_attrs,
  [EL_OL] = _ol_attrs,
  [EL_OPTGROUP] = _optgroup_attrs,
  [EL_OPTION] = _option_attrs,
  [EL_OUTPUT] = _output_attrs,
  [EL_P] = _p_attrs,
  [EL_PROGRESS] = _progress_attrs,
  [EL_SCRIPT] = _script_attrs,
  [EL_SELECT] = _select_attrs,
  [EL_SOURCE] = _source_attrs,
  [EL_STYLE] = _style_attrs,
  [EL_TD] = _td_attrs,
  [EL_TEXTAREA] = _textarea_attrs,
  [EL_TH] = _th_attrs,
  [EL_TIME] = _time_attrs,
  [EL_TRACK] = _track_attrs,
  [EL_VIDEO] = _video_attrs,
};

static map(char, HTMLElement) _elements_by_name;

/* -- Memory ---------------------------------------------------------------- */

static void *eval_malloc(usize size) {
  return mem_dyn_arena_malloc(eval_arena, size);
}

static void *eval_calloc(usize count, usize size) {
  return mem_dyn_arena_calloc(eval_arena, count, size);
}

#define eval_new(T) ((T*)(eval_calloc(1, sizeof(T))))
#define eval_new_array(T, n) ((T*)(eval_calloc(n, sizeof(T))))

static void *builtins_malloc(usize size) {
  return mem_dyn_arena_malloc(builtins_arena, size);
}

static void *builtins_calloc(usize count, usize size) {
  return mem_dyn_arena_calloc(builtins_arena, count, size);
}

#define b_new(T) ((T*)(builtins_calloc(1, sizeof(T))))
#define b_new_array(T, n) ((T*)(builtins_calloc(n, sizeof(T))))

#define b_make(T, name, ...)                \
  T *name = b_new(T);                       \
  *name = (T)__VA_ARGS__

/* -- Builtins: utils ------------------------------------------------------- */

static Type *_builtin(BuiltinType builtin) {
  b_make(Type, type, { .type = T_BUILTIN, .builtin = builtin, .meta = {0} });
  return type;
}

static Type *_list_of(Type *item_type) {
  b_make(ListType, list, { .item_type = item_type, .meta = {0} });
  b_make(Type, type, { .type = T_LIST, .list = list, .meta = {0} });
  return type;
}

static Type *_tuple_of(Type **types, u32 len) {
  b_make(TupleType, tuple, {
    .types = types,
    .len = len,
    .meta = {0},
  });
  b_make(Type, type, { .type = T_TUPLE, .tuple = tuple, .meta = {0} });
  return type;
}

static Type *_vector_of(Type *item_type, u32 count) {
  b_make(VectorType, vec, {
    .item_type = item_type,
    .len = count,
    .meta = {0},
  });
  b_make(Type, type, { .type = T_VECTOR, .vec = vec, .meta = {0} });
  return type;
}

static Type *_map_of(Type *key_type, Type *value_type) {
  b_make(MapType, map, {
    .key_type = key_type,
    .value_type = value_type,
    .meta = {0}
  });
  b_make(Type, type, { .type = T_MAP, .map = map, .meta = {0} });
  return type;
}

static Type *_union_of(u32 n, ...) {
  b_make(UnionType, union_type, {
    .len = n,
    .types = b_new_array(Type *, n),
    .meta = {0},
  });

  va_list args;
  va_start(args, n);
  for (u32 i = 0; i < n; i++) union_type->types[i] = va_arg(args, Type *);
  va_end(args);

  b_make(Type, type, {
    .type = T_UNION,
    .union_type = union_type,
    .meta = {0}
  });
  return type;
}


static SymbolKey _sym_key(const char *name) {
  return (SymbolKey){ .name = (char *)name, .scope = cs()->global_scope.id };
}

static Identifier *static_id(const char *name) {
  Identifier *id = b_new(Identifier);
  *id = (Identifier){
    .label = IDENTIFIER,
    .len = strlen(name),
    .lexeme = (char *)name,
    .loc = {0},
  };

  return id;
}

static const Value *_to_list(const Value *val) {
  if (val->type == VAL_LIST) return val;

  Value *list_v = eval_new(Value);
  *list_v = (Value){ .type = VAL_LIST, .list = eval_new(List) };
  *list_v->list = (List){
    .len = 1,
    .items = eval_new_array(const Value *, 1),
  };
  list_v->list->items[0] = val;

  return list_v;
}

/* -- Module init/shutdown -------------------------------------------------- */

void builtins_init() {
  builtins_arena = mem_dyn_arena_create(MEM_SIZE_AUTO);

  _attr_string_type = _union_of(6,
    _builtin(T_INT),
    _builtin(T_FLOAT),
    _builtin(T_STRING),
    _builtin(T_BOOL),
    _list_of(_union_of(5,
      _builtin(T_INT),
      _builtin(T_FLOAT),
      _builtin(T_STRING),
      _builtin(T_BOOL),
      _builtin(T_NIL)
    )),
    _builtin(T_NIL)
  );
  _attr_bool_type = _union_of(2,
    _builtin(T_BOOL),
    _builtin(T_NIL)
  );
  _attr_map_type = _union_of(2,
    _map_of(_builtin(T_STRING), _attr_string_type),
    _builtin(T_NIL)
  );

  map_inita(&_elements_by_name, hash_cstring, compare_cstring, builtins_calloc, NULL);
  for (HTMLElementName el = 0; el < HTML_ELEM_COUNT; el++) {
    map_insert(&_elements_by_name, (char *)_elements[el].tag, &_elements[el]);
  }
}

void builtins_shutdown() {
  mem_dyn_arena_destroy(builtins_arena);
}

/* -- Builtins: functions --------------------------------------------------- */

static const Value *rand_fn(
  const Value **args,
  const Value *c,
  const FunctionDef *fn,
  const FunctionCallExpr *call
) {
  Value *value = eval_new(Value);
  *value = (Value){ .type = VAL_INT, .integer = rand() };
  return value;
}

static void _register_rand_fn(SymbolTable *s) {
  FunctionDef *def = b_new(FunctionDef);
  *def = (FunctionDef){
    .meta = {0},
    .name = static_id("rand"),
    .params_len = 0,
    .params = NULL,
    .compose_type = NULL,
    .return_type = _builtin(T_INT),
    .builtin = rand_fn,
  };

  SymbolKey key = _sym_key("rand");
  bool inserted = map_insert(s, &key, sym_function(def));
  assert(inserted);
}

static const Value *element_fn(
  const Value **args,
  const Value *c,
  const FunctionDef *fn,
  const FunctionCallExpr *call
) {
  const HTMLElement *el_def = map_get(&_elements_by_name, fn->name->lexeme);

  Element *element = eval_new(Element);
  *element = (Element){
    .tag = fn->name->lexeme,
    .self_closing = el_def->self_closing,
    .attributes = eval_new(ElementAttributes),
    .children = c ? _to_list(c)->list : NULL,
  };
  map_inita(element->attributes, hash_cstring, compare_cstring, eval_calloc, NULL);

  for (u32 i = 0; i < fn->params_len; i++) {
    Parameter *p = fn->params[i];
    for (u32 i = 0; i < call->args_len; i++) {
      const Identifier *name = or_default(call->args[i]->name, fn->params[i]->name);
      if (args[i]->type == VAL_MAP) {
        str base_name = str_from_cstring_a(eval_malloc, name->lexeme);
        str dash = str_from_cstring_a(eval_malloc, "-");

        const Value *attr_key;
        const Value *attr_value;
        map_foreach(attr_key, attr_value, args[i]->map) {
          str attr_name = str_concat_a(eval_malloc, 2, dash, base_name, *attr_key->string);
          char *attr_name_c = str_to_cstring_a(eval_malloc, attr_name);
          map_insert(element->attributes, attr_name_c, (Value *)attr_value);
        }
      } else {
        map_insert(element->attributes, name->lexeme, (Value *)args[i]);
      }
    }
  }

  Value *value = eval_new(Value);
  *value = (Value){ .type = VAL_ELEMENT, .element = element };
  return value;
}

static Type *_get_attr_param_type(BuiltinType type) {
  switch (type) {
    case T_STRING: return _attr_string_type;
    case T_BOOL: return _attr_bool_type;
    case T_VOID: return _attr_map_type; // Interpreted as map
    default: assert(false);
  }
}

static Parameter *_get_param_for_attr(HTMLAttributeName attr_name) {
  HTMLAttribute *attr = &_attributes[attr_name];
  Parameter *param = b_new(Parameter);
  *param = (Parameter){
    .meta = {0},
    .name = static_id(attr->name),
    .type = _get_attr_param_type(attr->type),
    .default_value = NULL,
  };

  return param;
}

static Parameter **_get_param_list_for_element(HTMLElementName el, u32 *len) {
  HTMLAttributeName *element_attrs = _attrs_per_element[el];

  // Get length
  u32 global_attr_count = sizeof(_global_attrs) / sizeof(*_global_attrs);
  *len = global_attr_count;
  if (element_attrs) {
    for (HTMLAttributeName *attr = element_attrs; *attr != -1; attr++) {
      (*len)++;
    }
  }

  Parameter **params = b_new_array(Parameter *, *len);
  u32 i;
  for (i = 0; i < global_attr_count; i++) {
    params[i] = _get_param_for_attr(_global_attrs[i]);
  }
  if (element_attrs) {
    for (HTMLAttributeName *attr = element_attrs; *attr != -1; attr++) {
      params[i++] = _get_param_for_attr(*attr);
    }
  }
  return params;
}

static void _register_element_fn(
  SymbolTable *s,
  HTMLElementName el
) {
  HTMLElement *element = &_elements[el];

  u32 params_len;
  Parameter **params = _get_param_list_for_element(el, &params_len);
  Type *children_type = _union_of(7,
    _builtin(T_INT),
    _builtin(T_FLOAT),
    _builtin(T_BOOL),
    _builtin(T_STRING),
    _builtin(T_NIL),
    _builtin(T_ELEMENT),
    _list_of(NULL)
  );
  children_type->union_type->types[6]->list->item_type = children_type;

  FunctionDef *def = b_new(FunctionDef);
  *def = (FunctionDef){
    .meta = {0},
    .name = static_id(element->tag),
    .params_len = params_len,
    .params = params,
    .compose_type = element->self_closing ? NULL : children_type,
    .return_type = _builtin(T_ELEMENT),
    .builtin = element_fn,
  };

  SymbolKey key = _sym_key(element->tag);
  bool inserted = map_insert(s, &key, sym_function(def));
  assert(inserted );
}

/* -- Builtins: API --------------------------------------------------------- */

void builtins_register_fns(SymbolTable *s) {
  _register_rand_fn(s);

  for (HTMLElementName el = 0; el < HTML_ELEM_COUNT; el++) {
    _register_element_fn(s, el);
  }
}

void builtins_set_eval_arena(dyn_arena *a) {
  eval_arena = a;
}

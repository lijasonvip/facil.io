/*
Copyright: Boaz Segev, 2016-2019
License: MIT

Feel free to copy, use and enjoy according to the license provided.
*/
#include <http_internal.h>

#include <http1.h>

/* *****************************************************************************
Internal Request / Response Handlers
***************************************************************************** */

/** Use this function to handle HTTP requests.*/
void http_on_request_handler______internal(http_s *h,
                                           http_settings_s *settings) {

  /* Set udata for the request, in case user made changes in previous request */
  h->udata = settings->udata;

  if (1) {
    /* test for Host header and avoid duplicates */
    FIOBJ tmp = fiobj_hash_get2(h->headers, HTTP_HEADER_HOST);
    if (!tmp)
      goto missing_host;
    if (FIOBJ_TYPE_IS(tmp, FIOBJ_T_ARRAY)) {
      fiobj_array_pop(tmp, &tmp);
      fiobj_hash_set2(h->headers, HTTP_HEADER_HOST, tmp);
    }
  }

  FIOBJ t = fiobj_hash_get2(h->headers, HTTP_HEADER_UPGRADE);
  if (t)
    goto upgrade;

  if (fiobj_is_eq(fiobj_hash_get2(h->headers, HTTP_HEADER_ACCEPT),
                  HTTP_HVALUE_SSE_MIME))
    goto eventsource;
  if (settings->public_folder) {
    fio_str_info_s path_str = fiobj2cstr(h->path);
    if (!http_sendfile2(h,
                        settings->public_folder,
                        settings->public_folder_length,
                        path_str.buf,
                        path_str.len)) {
      return;
    }
  }
  settings->on_request(h);
  return;

upgrade:
  if (1) {
    fiobj_dup(t); /* allow upgrade name access after http_finish */
    fio_str_info_s val = fiobj2cstr(t);
    if (val.buf[0] == 'h' && val.buf[1] == '2') {
      http_send_error(h, 400);
    } else {
      settings->on_upgrade(h, val.buf, val.len);
    }
    fiobj_free(t);
    return;
  }
eventsource:
  settings->on_upgrade(h, (char *)"sse", 3);
  return;
missing_host:
  FIO_LOG_DEBUG("missing Host header");
  http_send_error(h, 400);
  return;
}

void http_on_response_handler______internal(http_s *h,
                                            http_settings_s *settings) {
  h->udata = settings->udata;
  FIOBJ t = fiobj_hash_get2(h->headers, HTTP_HEADER_UPGRADE);
  if (t == FIOBJ_INVALID) {
    settings->on_response(h);
    return;
  } else {
    fio_str_info_s val = fiobj2cstr(t);
    settings->on_upgrade(h, val.buf, val.len);
  }
}

/* *****************************************************************************
Internal helpers
***************************************************************************** */

int http_send_error2(size_t error, intptr_t uuid, http_settings_s *settings) {
  if (!uuid || !settings || !error)
    return -1;
  fio_protocol_s *pr = http1_new(uuid, settings, NULL, 0);
  http_internal_s *h = fio_malloc(sizeof(*h));
  FIO_ASSERT(pr, "Couldn't allocate response object for error report.")
  *h = (http_internal_s)HTTP_H_INIT(http1_vtable(), (http_fio_protocol_s *)pr);
  int ret = http_send_error(&h->public, error);
  fio_close(uuid);
  return ret;
}

/* *****************************************************************************
Library initialization
***************************************************************************** */

FIOBJ HTTP_HEADER_ACCEPT;
FIOBJ HTTP_HEADER_ACCEPT_ENCODING;
FIOBJ HTTP_HEADER_ACCEPT_RANGES;
FIOBJ HTTP_HEADER_ALLOW;
FIOBJ HTTP_HEADER_CACHE_CONTROL;
FIOBJ HTTP_HEADER_CONNECTION;
FIOBJ HTTP_HEADER_CONTENT_ENCODING;
FIOBJ HTTP_HEADER_CONTENT_LENGTH;
FIOBJ HTTP_HEADER_CONTENT_RANGE;
FIOBJ HTTP_HEADER_CONTENT_TYPE;
FIOBJ HTTP_HEADER_COOKIE;
FIOBJ HTTP_HEADER_DATE;
FIOBJ HTTP_HEADER_ETAG;
FIOBJ HTTP_HEADER_HOST;
FIOBJ HTTP_HEADER_IF_NONE_MATCH;
FIOBJ HTTP_HEADER_IF_RANGE;
FIOBJ HTTP_HEADER_LAST_MODIFIED;
FIOBJ HTTP_HEADER_ORIGIN;
FIOBJ HTTP_HEADER_RANGE;
FIOBJ HTTP_HEADER_SET_COOKIE;
FIOBJ HTTP_HEADER_TRANSFER_ENCODING;
FIOBJ HTTP_HEADER_UPGRADE;
FIOBJ HTTP_HEADER_WS_SEC_CLIENT_KEY;
FIOBJ HTTP_HEADER_WS_SEC_KEY;
FIOBJ HTTP_HVALUE_BYTES;
FIOBJ HTTP_HVALUE_CHUNKED_ENCODING;
FIOBJ HTTP_HVALUE_CLOSE;
FIOBJ HTTP_HVALUE_CONTENT_TYPE_DEFAULT;
FIOBJ HTTP_HVALUE_GZIP;
FIOBJ HTTP_HVALUE_KEEP_ALIVE;
FIOBJ HTTP_HVALUE_MAX_AGE;
FIOBJ HTTP_HVALUE_NO_CACHE;
FIOBJ HTTP_HVALUE_SSE_MIME;
FIOBJ HTTP_HVALUE_WEBSOCKET;
FIOBJ HTTP_HVALUE_WS_SEC_VERSION;
FIOBJ HTTP_HVALUE_WS_UPGRADE;
FIOBJ HTTP_HVALUE_WS_VERSION;

static void http_lib_init(void *ignr_);
static void http_lib_cleanup(void *ignr_);
static __attribute__((constructor)) void http_lib_constructor(void) {
  fio_state_callback_add(FIO_CALL_ON_INITIALIZE, http_lib_init, NULL);
  fio_state_callback_add(FIO_CALL_AT_EXIT, http_lib_cleanup, NULL);
}

void http_mimetype_stats(void);

static void http_lib_cleanup(void *ignr_) {
  FIO_LOG_DEBUG("Freeing HTTP extension resources");
  (void)ignr_;
  http_mimetype_clear();
#define HTTPLIB_RESET(x)                                                       \
  fiobj_free(x);                                                               \
  x = FIOBJ_INVALID;
  HTTPLIB_RESET(HTTP_HEADER_ACCEPT);
  HTTPLIB_RESET(HTTP_HEADER_ACCEPT_ENCODING);
  HTTPLIB_RESET(HTTP_HEADER_ACCEPT_RANGES);
  HTTPLIB_RESET(HTTP_HEADER_ALLOW);
  HTTPLIB_RESET(HTTP_HEADER_CACHE_CONTROL);
  HTTPLIB_RESET(HTTP_HEADER_CONNECTION);
  HTTPLIB_RESET(HTTP_HEADER_CONTENT_ENCODING);
  HTTPLIB_RESET(HTTP_HEADER_CONTENT_LENGTH);
  HTTPLIB_RESET(HTTP_HEADER_CONTENT_RANGE);
  HTTPLIB_RESET(HTTP_HEADER_CONTENT_TYPE);
  HTTPLIB_RESET(HTTP_HEADER_COOKIE);
  HTTPLIB_RESET(HTTP_HEADER_DATE);
  HTTPLIB_RESET(HTTP_HEADER_ETAG);
  HTTPLIB_RESET(HTTP_HEADER_HOST);
  HTTPLIB_RESET(HTTP_HEADER_IF_NONE_MATCH);
  HTTPLIB_RESET(HTTP_HEADER_IF_RANGE);
  HTTPLIB_RESET(HTTP_HEADER_LAST_MODIFIED);
  HTTPLIB_RESET(HTTP_HEADER_ORIGIN);
  HTTPLIB_RESET(HTTP_HEADER_RANGE);
  HTTPLIB_RESET(HTTP_HEADER_SET_COOKIE);
  HTTPLIB_RESET(HTTP_HEADER_TRANSFER_ENCODING);
  HTTPLIB_RESET(HTTP_HEADER_UPGRADE);
  HTTPLIB_RESET(HTTP_HEADER_WS_SEC_CLIENT_KEY);
  HTTPLIB_RESET(HTTP_HEADER_WS_SEC_KEY);
  HTTPLIB_RESET(HTTP_HVALUE_BYTES);
  HTTPLIB_RESET(HTTP_HVALUE_CHUNKED_ENCODING);
  HTTPLIB_RESET(HTTP_HVALUE_CLOSE);
  HTTPLIB_RESET(HTTP_HVALUE_CONTENT_TYPE_DEFAULT);
  HTTPLIB_RESET(HTTP_HVALUE_GZIP);
  HTTPLIB_RESET(HTTP_HVALUE_KEEP_ALIVE);
  HTTPLIB_RESET(HTTP_HVALUE_MAX_AGE);
  HTTPLIB_RESET(HTTP_HVALUE_NO_CACHE);
  HTTPLIB_RESET(HTTP_HVALUE_SSE_MIME);
  HTTPLIB_RESET(HTTP_HVALUE_WEBSOCKET);
  HTTPLIB_RESET(HTTP_HVALUE_WS_SEC_VERSION);
  HTTPLIB_RESET(HTTP_HVALUE_WS_UPGRADE);
  HTTPLIB_RESET(HTTP_HVALUE_WS_VERSION);
#undef HTTPLIB_RESET

  http_mimetype_stats();
}

static void http_lib_init(void *ignr_) {
  (void)ignr_;
  if (HTTP_HEADER_ACCEPT_RANGES)
    return;

  HTTP_HEADER_ACCEPT = fiobj_str_new_cstr("accept", 6);
  HTTP_HEADER_ACCEPT_ENCODING = fiobj_str_new_cstr("accept-encoding", 15);
  HTTP_HEADER_ACCEPT_RANGES = fiobj_str_new_cstr("accept-ranges", 13);
  HTTP_HEADER_ALLOW = fiobj_str_new_cstr("allow", 5);
  HTTP_HEADER_CACHE_CONTROL = fiobj_str_new_cstr("cache-control", 13);
  HTTP_HEADER_CONNECTION = fiobj_str_new_cstr("connection", 10);
  HTTP_HEADER_CONTENT_ENCODING = fiobj_str_new_cstr("content-encoding", 16);
  HTTP_HEADER_CONTENT_LENGTH = fiobj_str_new_cstr("content-length", 14);
  HTTP_HEADER_CONTENT_RANGE = fiobj_str_new_cstr("content-range", 13);
  HTTP_HEADER_CONTENT_TYPE = fiobj_str_new_cstr("content-type", 12);
  HTTP_HEADER_COOKIE = fiobj_str_new_cstr("cookie", 6);
  HTTP_HEADER_DATE = fiobj_str_new_cstr("date", 4);
  HTTP_HEADER_ETAG = fiobj_str_new_cstr("etag", 4);
  HTTP_HEADER_HOST = fiobj_str_new_cstr("host", 4);
  HTTP_HEADER_IF_NONE_MATCH = fiobj_str_new_cstr("if-none-match", 13);
  HTTP_HEADER_IF_RANGE = fiobj_str_new_cstr("if-range", 8);
  HTTP_HEADER_LAST_MODIFIED = fiobj_str_new_cstr("last-modified", 13);
  HTTP_HEADER_ORIGIN = fiobj_str_new_cstr("origin", 6);
  HTTP_HEADER_RANGE = fiobj_str_new_cstr("range", 5);
  HTTP_HEADER_SET_COOKIE = fiobj_str_new_cstr("set-cookie", 10);
  HTTP_HEADER_TRANSFER_ENCODING = fiobj_str_new_cstr("transfer-encoding", 17);
  HTTP_HEADER_UPGRADE = fiobj_str_new_cstr("upgrade", 7);
  HTTP_HEADER_WS_SEC_CLIENT_KEY = fiobj_str_new_cstr("sec-websocket-key", 17);
  HTTP_HEADER_WS_SEC_KEY = fiobj_str_new_cstr("sec-websocket-accept", 20);
  HTTP_HVALUE_BYTES = fiobj_str_new_cstr("bytes", 5);
  HTTP_HVALUE_CHUNKED_ENCODING = fiobj_str_new_cstr("chunked", 7);
  HTTP_HVALUE_CLOSE = fiobj_str_new_cstr("close", 5);
  HTTP_HVALUE_CONTENT_TYPE_DEFAULT =
      fiobj_str_new_cstr("application/octet-stream", 24);
  HTTP_HVALUE_GZIP = fiobj_str_new_cstr("gzip", 4);
  HTTP_HVALUE_KEEP_ALIVE = fiobj_str_new_cstr("keep-alive", 10);
  HTTP_HVALUE_MAX_AGE = fiobj_str_new_cstr("max-age=3600", 12);
  HTTP_HVALUE_NO_CACHE = fiobj_str_new_cstr("no-cache, max-age=0", 19);
  HTTP_HVALUE_SSE_MIME = fiobj_str_new_cstr("text/event-stream", 17);
  HTTP_HVALUE_WEBSOCKET = fiobj_str_new_cstr("websocket", 9);
  HTTP_HVALUE_WS_SEC_VERSION = fiobj_str_new_cstr("sec-websocket-version", 21);
  HTTP_HVALUE_WS_UPGRADE = fiobj_str_new_cstr("Upgrade", 7);
  HTTP_HVALUE_WS_VERSION = fiobj_str_new_cstr("13", 2);

#define REGISTER_MIME(ext, type)                                               \
  http_mimetype_register((char *)ext,                                          \
                         sizeof(ext) - 1,                                      \
                         fiobj_str_new_cstr((char *)type, sizeof(type) - 1))

#if HTTP_MIME_REGISTRY_AUTO
  FIO_LOG_DEBUG2("(HTTP) Registering core mime-types");
  REGISTER_MIME("html", "text/html");
  REGISTER_MIME("txt", "text/plain");
  REGISTER_MIME("htm", "text/html");
  REGISTER_MIME("css", "text/css");
  REGISTER_MIME("js", "application/javascript");
  REGISTER_MIME("json", "application/json");
#endif

#if HTTP_MIME_REGISTRY_AUTO > 0
  FIO_LOG_DEBUG2("(HTTP) Registering all known mime-types");
  REGISTER_MIME("123", "application/vnd.lotus-1-2-3");
  REGISTER_MIME("3dml", "text/vnd.in3d.3dml");
  REGISTER_MIME("3ds", "image/x-3ds");
  REGISTER_MIME("3g2", "video/3gpp2");
  REGISTER_MIME("3gp", "video/3gpp");
  REGISTER_MIME("7z", "application/x-7z-compressed");
  REGISTER_MIME("aab", "application/x-authorware-bin");
  REGISTER_MIME("aac", "audio/x-aac");
  REGISTER_MIME("aam", "application/x-authorware-map");
  REGISTER_MIME("aas", "application/x-authorware-seg");
  REGISTER_MIME("abw", "application/x-abiword");
  REGISTER_MIME("ac", "application/pkix-attr-cert");
  REGISTER_MIME("acc", "application/vnd.americandynamics.acc");
  REGISTER_MIME("ace", "application/x-ace-compressed");
  REGISTER_MIME("acu", "application/vnd.acucobol");
  REGISTER_MIME("acutc", "application/vnd.acucorp");
  REGISTER_MIME("adp", "audio/adpcm");
  REGISTER_MIME("aep", "application/vnd.audiograph");
  REGISTER_MIME("afm", "application/x-font-type1");
  REGISTER_MIME("afp", "application/vnd.ibm.modcap");
  REGISTER_MIME("ahead", "application/vnd.ahead.space");
  REGISTER_MIME("ai", "application/postscript");
  REGISTER_MIME("aif", "audio/x-aiff");
  REGISTER_MIME("aifc", "audio/x-aiff");
  REGISTER_MIME("aiff", "audio/x-aiff");
  REGISTER_MIME("air",
                "application/vnd.adobe.air-application-installer-package+zip");
  REGISTER_MIME("ait", "application/vnd.dvb.ait");
  REGISTER_MIME("ami", "application/vnd.amiga.ami");
  REGISTER_MIME("apk", "application/vnd.android.package-archive");
  REGISTER_MIME("appcache", "text/cache-manifest");
  REGISTER_MIME("application", "application/x-ms-application");
  REGISTER_MIME("pptx",
                "application/"
                "vnd.openxmlformats-officedocument.presentationml."
                "presentation");
  REGISTER_MIME("apr", "application/vnd.lotus-approach");
  REGISTER_MIME("arc", "application/x-freearc");
  REGISTER_MIME("asc", "application/pgp-signature");
  REGISTER_MIME("asf", "video/x-ms-asf");
  REGISTER_MIME("asm", "text/x-asm");
  REGISTER_MIME("aso", "application/vnd.accpac.simply.aso");
  REGISTER_MIME("asx", "video/x-ms-asf");
  REGISTER_MIME("atc", "application/vnd.acucorp");
  REGISTER_MIME("atom", "application/atom+xml");
  REGISTER_MIME("atomcat", "application/atomcat+xml");
  REGISTER_MIME("atomsvc", "application/atomsvc+xml");
  REGISTER_MIME("atx", "application/vnd.antix.game-component");
  REGISTER_MIME("au", "audio/basic");
  REGISTER_MIME("avi", "video/x-msvideo");
  REGISTER_MIME("aw", "application/applixware");
  REGISTER_MIME("azf", "application/vnd.airzip.filesecure.azf");
  REGISTER_MIME("azs", "application/vnd.airzip.filesecure.azs");
  REGISTER_MIME("azw", "application/vnd.amazon.ebook");
  REGISTER_MIME("bat", "application/x-msdownload");
  REGISTER_MIME("bcpio", "application/x-bcpio");
  REGISTER_MIME("bdf", "application/x-font-bdf");
  REGISTER_MIME("bdm", "application/vnd.syncml.dm+wbxml");
  REGISTER_MIME("bed", "application/vnd.realvnc.bed");
  REGISTER_MIME("bh2", "application/vnd.fujitsu.oasysprs");
  REGISTER_MIME("bin", "application/octet-stream");
  REGISTER_MIME("blb", "application/x-blorb");
  REGISTER_MIME("blorb", "application/x-blorb");
  REGISTER_MIME("bmi", "application/vnd.bmi");
  REGISTER_MIME("bmp", "image/bmp");
  REGISTER_MIME("book", "application/vnd.framemaker");
  REGISTER_MIME("box", "application/vnd.previewsystems.box");
  REGISTER_MIME("boz", "application/x-bzip2");
  REGISTER_MIME("bpk", "application/octet-stream");
  REGISTER_MIME("btif", "image/prs.btif");
  REGISTER_MIME("bz", "application/x-bzip");
  REGISTER_MIME("bz2", "application/x-bzip2");
  REGISTER_MIME("c", "text/x-c");
  REGISTER_MIME("c11amc", "application/vnd.cluetrust.cartomobile-config");
  REGISTER_MIME("c11amz", "application/vnd.cluetrust.cartomobile-config-pkg");
  REGISTER_MIME("c4d", "application/vnd.clonk.c4group");
  REGISTER_MIME("c4f", "application/vnd.clonk.c4group");
  REGISTER_MIME("c4g", "application/vnd.clonk.c4group");
  REGISTER_MIME("c4p", "application/vnd.clonk.c4group");
  REGISTER_MIME("c4u", "application/vnd.clonk.c4group");
  REGISTER_MIME("cab", "application/vnd.ms-cab-compressed");
  REGISTER_MIME("caf", "audio/x-caf");
  REGISTER_MIME("cap", "application/vnd.tcpdump.pcap");
  REGISTER_MIME("car", "application/vnd.curl.car");
  REGISTER_MIME("cat", "application/vnd.ms-pki.seccat");
  REGISTER_MIME("cb7", "application/x-cbr");
  REGISTER_MIME("cba", "application/x-cbr");
  REGISTER_MIME("cbr", "application/x-cbr");
  REGISTER_MIME("cbt", "application/x-cbr");
  REGISTER_MIME("cbz", "application/x-cbr");
  REGISTER_MIME("cc", "text/x-c");
  REGISTER_MIME("cct", "application/x-director");
  REGISTER_MIME("ccxml", "application/ccxml+xml");
  REGISTER_MIME("cdbcmsg", "application/vnd.contact.cmsg");
  REGISTER_MIME("cdf", "application/x-netcdf");
  REGISTER_MIME("cdkey", "application/vnd.mediastation.cdkey");
  REGISTER_MIME("cdmia", "application/cdmi-capability");
  REGISTER_MIME("cdmic", "application/cdmi-container");
  REGISTER_MIME("cdmid", "application/cdmi-domain");
  REGISTER_MIME("cdmio", "application/cdmi-object");
  REGISTER_MIME("cdmiq", "application/cdmi-queue");
  REGISTER_MIME("cdx", "chemical/x-cdx");
  REGISTER_MIME("cdxml", "application/vnd.chemdraw+xml");
  REGISTER_MIME("cdy", "application/vnd.cinderella");
  REGISTER_MIME("cer", "application/pkix-cert");
  REGISTER_MIME("cfs", "application/x-cfs-compressed");
  REGISTER_MIME("cgm", "image/cgm");
  REGISTER_MIME("chat", "application/x-chat");
  REGISTER_MIME("chm", "application/vnd.ms-htmlhelp");
  REGISTER_MIME("chrt", "application/vnd.kde.kchart");
  REGISTER_MIME("cif", "chemical/x-cif");
  REGISTER_MIME("cii",
                "application/vnd.anser-web-certificate-issue-initiation");
  REGISTER_MIME("cil", "application/vnd.ms-artgalry");
  REGISTER_MIME("cla", "application/vnd.claymore");
  REGISTER_MIME("class", "application/java-vm");
  REGISTER_MIME("clkk", "application/vnd.crick.clicker.keyboard");
  REGISTER_MIME("clkp", "application/vnd.crick.clicker.palette");
  REGISTER_MIME("clkt", "application/vnd.crick.clicker.template");
  REGISTER_MIME("clkw", "application/vnd.crick.clicker.wordbank");
  REGISTER_MIME("clkx", "application/vnd.crick.clicker");
  REGISTER_MIME("clp", "application/x-msclip");
  REGISTER_MIME("cmc", "application/vnd.cosmocaller");
  REGISTER_MIME("cmdf", "chemical/x-cmdf");
  REGISTER_MIME("cml", "chemical/x-cml");
  REGISTER_MIME("cmp", "application/vnd.yellowriver-custom-menu");
  REGISTER_MIME("cmx", "image/x-cmx");
  REGISTER_MIME("cod", "application/vnd.rim.cod");
  REGISTER_MIME("com", "application/x-msdownload");
  REGISTER_MIME("conf", "text/plain");
  REGISTER_MIME("cpio", "application/x-cpio");
  REGISTER_MIME("cpp", "text/x-c");
  REGISTER_MIME("cpt", "application/mac-compactpro");
  REGISTER_MIME("crd", "application/x-mscardfile");
  REGISTER_MIME("crl", "application/pkix-crl");
  REGISTER_MIME("crt", "application/x-x509-ca-cert");
  REGISTER_MIME("cryptonote", "application/vnd.rig.cryptonote");
  REGISTER_MIME("csh", "application/x-csh");
  REGISTER_MIME("csml", "chemical/x-csml");
  REGISTER_MIME("csp", "application/vnd.commonspace");
  REGISTER_MIME("cst", "application/x-director");
  REGISTER_MIME("csv", "text/csv");
  REGISTER_MIME("cu", "application/cu-seeme");
  REGISTER_MIME("curl", "text/vnd.curl");
  REGISTER_MIME("cww", "application/prs.cww");
  REGISTER_MIME("cxt", "application/x-director");
  REGISTER_MIME("cxx", "text/x-c");
  REGISTER_MIME("dae", "model/vnd.collada+xml");
  REGISTER_MIME("daf", "application/vnd.mobius.daf");
  REGISTER_MIME("dart", "application/vnd.dart");
  REGISTER_MIME("dataless", "application/vnd.fdsn.seed");
  REGISTER_MIME("davmount", "application/davmount+xml");
  REGISTER_MIME("dbk", "application/docbook+xml");
  REGISTER_MIME("dcr", "application/x-director");
  REGISTER_MIME("dcurl", "text/vnd.curl.dcurl");
  REGISTER_MIME("dd2", "application/vnd.oma.dd2+xml");
  REGISTER_MIME("ddd", "application/vnd.fujixerox.ddd");
  REGISTER_MIME("deb", "application/x-debian-package");
  REGISTER_MIME("def", "text/plain");
  REGISTER_MIME("deploy", "application/octet-stream");
  REGISTER_MIME("der", "application/x-x509-ca-cert");
  REGISTER_MIME("dfac", "application/vnd.dreamfactory");
  REGISTER_MIME("dgc", "application/x-dgc-compressed");
  REGISTER_MIME("dic", "text/x-c");
  REGISTER_MIME("dir", "application/x-director");
  REGISTER_MIME("dis", "application/vnd.mobius.dis");
  REGISTER_MIME("dist", "application/octet-stream");
  REGISTER_MIME("distz", "application/octet-stream");
  REGISTER_MIME("djv", "image/vnd.djvu");
  REGISTER_MIME("djvu", "image/vnd.djvu");
  REGISTER_MIME("dll", "application/x-msdownload");
  REGISTER_MIME("dmg", "application/x-apple-diskimage");
  REGISTER_MIME("dmp", "application/vnd.tcpdump.pcap");
  REGISTER_MIME("dms", "application/octet-stream");
  REGISTER_MIME("dna", "application/vnd.dna");
  REGISTER_MIME("doc", "application/msword");
  REGISTER_MIME("docm", "application/vnd.ms-word.document.macroenabled.12");
  REGISTER_MIME("docx",
                "application/"
                "vnd.openxmlformats-officedocument.wordprocessingml."
                "document");
  REGISTER_MIME("dot", "application/msword");
  REGISTER_MIME("dotm", "application/vnd.ms-word.template.macroenabled.12");
  REGISTER_MIME("dotx",
                "application/"
                "vnd.openxmlformats-officedocument.wordprocessingml."
                "template");
  REGISTER_MIME("dp", "application/vnd.osgi.dp");
  REGISTER_MIME("dpg", "application/vnd.dpgraph");
  REGISTER_MIME("dra", "audio/vnd.dra");
  REGISTER_MIME("dsc", "text/prs.lines.tag");
  REGISTER_MIME("dssc", "application/dssc+der");
  REGISTER_MIME("dtb", "application/x-dtbook+xml");
  REGISTER_MIME("dtd", "application/xml-dtd");
  REGISTER_MIME("dts", "audio/vnd.dts");
  REGISTER_MIME("dtshd", "audio/vnd.dts.hd");
  REGISTER_MIME("dump", "application/octet-stream");
  REGISTER_MIME("dvb", "video/vnd.dvb.file");
  REGISTER_MIME("dvi", "application/x-dvi");
  REGISTER_MIME("dwf", "model/vnd.dwf");
  REGISTER_MIME("dwg", "image/vnd.dwg");
  REGISTER_MIME("dxf", "image/vnd.dxf");
  REGISTER_MIME("dxp", "application/vnd.spotfire.dxp");
  REGISTER_MIME("dxr", "application/x-director");
  REGISTER_MIME("ecelp4800", "audio/vnd.nuera.ecelp4800");
  REGISTER_MIME("ecelp7470", "audio/vnd.nuera.ecelp7470");
  REGISTER_MIME("ecelp9600", "audio/vnd.nuera.ecelp9600");
  REGISTER_MIME("ecma", "application/ecmascript");
  REGISTER_MIME("edm", "application/vnd.novadigm.edm");
  REGISTER_MIME("edx", "application/vnd.novadigm.edx");
  REGISTER_MIME("efif", "application/vnd.picsel");
  REGISTER_MIME("ei6", "application/vnd.pg.osasli");
  REGISTER_MIME("elc", "application/octet-stream");
  REGISTER_MIME("emf", "application/x-msmetafile");
  REGISTER_MIME("eml", "message/rfc822");
  REGISTER_MIME("emma", "application/emma+xml");
  REGISTER_MIME("emz", "application/x-msmetafile");
  REGISTER_MIME("eol", "audio/vnd.digital-winds");
  REGISTER_MIME("eot", "application/vnd.ms-fontobject");
  REGISTER_MIME("eps", "application/postscript");
  REGISTER_MIME("epub", "application/epub+zip");
  REGISTER_MIME("es3", "application/vnd.eszigno3+xml");
  REGISTER_MIME("esa", "application/vnd.osgi.subsystem");
  REGISTER_MIME("esf", "application/vnd.epson.esf");
  REGISTER_MIME("et3", "application/vnd.eszigno3+xml");
  REGISTER_MIME("etx", "text/x-setext");
  REGISTER_MIME("eva", "application/x-eva");
  REGISTER_MIME("evy", "application/x-envoy");
  REGISTER_MIME("exe", "application/x-msdownload");
  REGISTER_MIME("exi", "application/exi");
  REGISTER_MIME("ext", "application/vnd.novadigm.ext");
  REGISTER_MIME("ez", "application/andrew-inset");
  REGISTER_MIME("ez2", "application/vnd.ezpix-album");
  REGISTER_MIME("ez3", "application/vnd.ezpix-package");
  REGISTER_MIME("f", "text/x-fortran");
  REGISTER_MIME("f4v", "video/x-f4v");
  REGISTER_MIME("f77", "text/x-fortran");
  REGISTER_MIME("f90", "text/x-fortran");
  REGISTER_MIME("fbs", "image/vnd.fastbidsheet");
  REGISTER_MIME("fcdt", "application/vnd.adobe.formscentral.fcdt");
  REGISTER_MIME("fcs", "application/vnd.isac.fcs");
  REGISTER_MIME("fdf", "application/vnd.fdf");
  REGISTER_MIME("fe_launch", "application/vnd.denovo.fcselayout-link");
  REGISTER_MIME("fg5", "application/vnd.fujitsu.oasysgp");
  REGISTER_MIME("fgd", "application/x-director");
  REGISTER_MIME("fh", "image/x-freehand");
  REGISTER_MIME("fh4", "image/x-freehand");
  REGISTER_MIME("fh5", "image/x-freehand");
  REGISTER_MIME("fh7", "image/x-freehand");
  REGISTER_MIME("fhc", "image/x-freehand");
  REGISTER_MIME("fig", "application/x-xfig");
  REGISTER_MIME("flac", "audio/x-flac");
  REGISTER_MIME("fli", "video/x-fli");
  REGISTER_MIME("flo", "application/vnd.micrografx.flo");
  REGISTER_MIME("flv", "video/x-flv");
  REGISTER_MIME("flw", "application/vnd.kde.kivio");
  REGISTER_MIME("flx", "text/vnd.fmi.flexstor");
  REGISTER_MIME("fly", "text/vnd.fly");
  REGISTER_MIME("fm", "application/vnd.framemaker");
  REGISTER_MIME("fnc", "application/vnd.frogans.fnc");
  REGISTER_MIME("for", "text/x-fortran");
  REGISTER_MIME("fpx", "image/vnd.fpx");
  REGISTER_MIME("frame", "application/vnd.framemaker");
  REGISTER_MIME("fsc", "application/vnd.fsc.weblaunch");
  REGISTER_MIME("fst", "image/vnd.fst");
  REGISTER_MIME("ftc", "application/vnd.fluxtime.clip");
  REGISTER_MIME("fti", "application/vnd.anser-web-funds-transfer-initiation");
  REGISTER_MIME("fvt", "video/vnd.fvt");
  REGISTER_MIME("fxp", "application/vnd.adobe.fxp");
  REGISTER_MIME("fxpl", "application/vnd.adobe.fxp");
  REGISTER_MIME("fzs", "application/vnd.fuzzysheet");
  REGISTER_MIME("g2w", "application/vnd.geoplan");
  REGISTER_MIME("g3", "image/g3fax");
  REGISTER_MIME("g3w", "application/vnd.geospace");
  REGISTER_MIME("gac", "application/vnd.groove-account");
  REGISTER_MIME("gam", "application/x-tads");
  REGISTER_MIME("gbr", "application/rpki-ghostbusters");
  REGISTER_MIME("gca", "application/x-gca-compressed");
  REGISTER_MIME("gdl", "model/vnd.gdl");
  REGISTER_MIME("geo", "application/vnd.dynageo");
  REGISTER_MIME("gex", "application/vnd.geometry-explorer");
  REGISTER_MIME("ggb", "application/vnd.geogebra.file");
  REGISTER_MIME("ggt", "application/vnd.geogebra.tool");
  REGISTER_MIME("ghf", "application/vnd.groove-help");
  REGISTER_MIME("gif", "image/gif");
  REGISTER_MIME("gim", "application/vnd.groove-identity-message");
  REGISTER_MIME("gml", "application/gml+xml");
  REGISTER_MIME("gmx", "application/vnd.gmx");
  REGISTER_MIME("gnumeric", "application/x-gnumeric");
  REGISTER_MIME("gph", "application/vnd.flographit");
  REGISTER_MIME("gpx", "application/gpx+xml");
  REGISTER_MIME("gqf", "application/vnd.grafeq");
  REGISTER_MIME("gqs", "application/vnd.grafeq");
  REGISTER_MIME("gram", "application/srgs");
  REGISTER_MIME("gramps", "application/x-gramps-xml");
  REGISTER_MIME("gre", "application/vnd.geometry-explorer");
  REGISTER_MIME("grv", "application/vnd.groove-injector");
  REGISTER_MIME("grxml", "application/srgs+xml");
  REGISTER_MIME("gsf", "application/x-font-ghostscript");
  REGISTER_MIME("gtar", "application/x-gtar");
  REGISTER_MIME("gtm", "application/vnd.groove-tool-message");
  REGISTER_MIME("gtw", "model/vnd.gtw");
  REGISTER_MIME("gv", "text/vnd.graphviz");
  REGISTER_MIME("gxf", "application/gxf");
  REGISTER_MIME("gxt", "application/vnd.geonext");
  REGISTER_MIME("h", "text/x-c");
  REGISTER_MIME("h261", "video/h261");
  REGISTER_MIME("h263", "video/h263");
  REGISTER_MIME("h264", "video/h264");
  REGISTER_MIME("hal", "application/vnd.hal+xml");
  REGISTER_MIME("hbci", "application/vnd.hbci");
  REGISTER_MIME("hdf", "application/x-hdf");
  REGISTER_MIME("hh", "text/x-c");
  REGISTER_MIME("hlp", "application/winhlp");
  REGISTER_MIME("hpgl", "application/vnd.hp-hpgl");
  REGISTER_MIME("hpid", "application/vnd.hp-hpid");
  REGISTER_MIME("hps", "application/vnd.hp-hps");
  REGISTER_MIME("hqx", "application/mac-binhex40");
  REGISTER_MIME("htke", "application/vnd.kenameaapp");
  REGISTER_MIME("hvd", "application/vnd.yamaha.hv-dic");
  REGISTER_MIME("hvp", "application/vnd.yamaha.hv-voice");
  REGISTER_MIME("hvs", "application/vnd.yamaha.hv-script");
  REGISTER_MIME("i2g", "application/vnd.intergeo");
  REGISTER_MIME("icc", "application/vnd.iccprofile");
  REGISTER_MIME("ice", "x-conference/x-cooltalk");
  REGISTER_MIME("icm", "application/vnd.iccprofile");
  REGISTER_MIME("ico", "image/x-icon");
  REGISTER_MIME("ics", "text/calendar");
  REGISTER_MIME("ief", "image/ief");
  REGISTER_MIME("ifb", "text/calendar");
  REGISTER_MIME("ifm", "application/vnd.shana.informed.formdata");
  REGISTER_MIME("iges", "model/iges");
  REGISTER_MIME("igl", "application/vnd.igloader");
  REGISTER_MIME("igm", "application/vnd.insors.igm");
  REGISTER_MIME("igs", "model/iges");
  REGISTER_MIME("igx", "application/vnd.micrografx.igx");
  REGISTER_MIME("iif", "application/vnd.shana.informed.interchange");
  REGISTER_MIME("imp", "application/vnd.accpac.simply.imp");
  REGISTER_MIME("ims", "application/vnd.ms-ims");
  REGISTER_MIME("in", "text/plain");
  REGISTER_MIME("ink", "application/inkml+xml");
  REGISTER_MIME("inkml", "application/inkml+xml");
  REGISTER_MIME("install", "application/x-install-instructions");
  REGISTER_MIME("iota", "application/vnd.astraea-software.iota");
  REGISTER_MIME("ipfix", "application/ipfix");
  REGISTER_MIME("ipk", "application/vnd.shana.informed.package");
  REGISTER_MIME("irm", "application/vnd.ibm.rights-management");
  REGISTER_MIME("irp", "application/vnd.irepository.package+xml");
  REGISTER_MIME("iso", "application/x-iso9660-image");
  REGISTER_MIME("itp", "application/vnd.shana.informed.formtemplate");
  REGISTER_MIME("ivp", "application/vnd.immervision-ivp");
  REGISTER_MIME("ivu", "application/vnd.immervision-ivu");
  REGISTER_MIME("jad", "text/vnd.sun.j2me.app-descriptor");
  REGISTER_MIME("jam", "application/vnd.jam");
  REGISTER_MIME("jar", "application/java-archive");
  REGISTER_MIME("java", "text/x-java-source");
  REGISTER_MIME("jisp", "application/vnd.jisp");
  REGISTER_MIME("jlt", "application/vnd.hp-jlyt");
  REGISTER_MIME("jnlp", "application/x-java-jnlp-file");
  REGISTER_MIME("joda", "application/vnd.joost.joda-archive");
  REGISTER_MIME("jpe", "image/jpeg");
  REGISTER_MIME("jpeg", "image/jpeg");
  REGISTER_MIME("jpg", "image/jpeg");
  REGISTER_MIME("jpgm", "video/jpm");
  REGISTER_MIME("jpgv", "video/jpeg");
  REGISTER_MIME("jpm", "video/jpm");
  REGISTER_MIME("jsonml", "application/jsonml+json");
  REGISTER_MIME("kar", "audio/midi");
  REGISTER_MIME("karbon", "application/vnd.kde.karbon");
  REGISTER_MIME("kfo", "application/vnd.kde.kformula");
  REGISTER_MIME("kia", "application/vnd.kidspiration");
  REGISTER_MIME("kml", "application/vnd.google-earth.kml+xml");
  REGISTER_MIME("kmz", "application/vnd.google-earth.kmz");
  REGISTER_MIME("kne", "application/vnd.kinar");
  REGISTER_MIME("knp", "application/vnd.kinar");
  REGISTER_MIME("kon", "application/vnd.kde.kontour");
  REGISTER_MIME("kpr", "application/vnd.kde.kpresenter");
  REGISTER_MIME("kpt", "application/vnd.kde.kpresenter");
  REGISTER_MIME("kpxx", "application/vnd.ds-keypoint");
  REGISTER_MIME("ksp", "application/vnd.kde.kspread");
  REGISTER_MIME("ktr", "application/vnd.kahootz");
  REGISTER_MIME("ktx", "image/ktx");
  REGISTER_MIME("ktz", "application/vnd.kahootz");
  REGISTER_MIME("kwd", "application/vnd.kde.kword");
  REGISTER_MIME("kwt", "application/vnd.kde.kword");
  REGISTER_MIME("lasxml", "application/vnd.las.las+xml");
  REGISTER_MIME("latex", "application/x-latex");
  REGISTER_MIME("lbd", "application/vnd.llamagraphics.life-balance.desktop");
  REGISTER_MIME("lbe",
                "application/vnd.llamagraphics.life-balance.exchange+xml");
  REGISTER_MIME("les", "application/vnd.hhe.lesson-player");
  REGISTER_MIME("lha", "application/x-lzh-compressed");
  REGISTER_MIME("link66", "application/vnd.route66.link66+xml");
  REGISTER_MIME("list", "text/plain");
  REGISTER_MIME("list3820", "application/vnd.ibm.modcap");
  REGISTER_MIME("listafp", "application/vnd.ibm.modcap");
  REGISTER_MIME("lnk", "application/x-ms-shortcut");
  REGISTER_MIME("log", "text/plain");
  REGISTER_MIME("lostxml", "application/lost+xml");
  REGISTER_MIME("lrf", "application/octet-stream");
  REGISTER_MIME("lrm", "application/vnd.ms-lrm");
  REGISTER_MIME("ltf", "application/vnd.frogans.ltf");
  REGISTER_MIME("lvp", "audio/vnd.lucent.voice");
  REGISTER_MIME("lwp", "application/vnd.lotus-wordpro");
  REGISTER_MIME("lzh", "application/x-lzh-compressed");
  REGISTER_MIME("m13", "application/x-msmediaview");
  REGISTER_MIME("m14", "application/x-msmediaview");
  REGISTER_MIME("m1v", "video/mpeg");
  REGISTER_MIME("m21", "application/mp21");
  REGISTER_MIME("m2a", "audio/mpeg");
  REGISTER_MIME("m2v", "video/mpeg");
  REGISTER_MIME("m3a", "audio/mpeg");
  REGISTER_MIME("m3u", "audio/x-mpegurl");
  REGISTER_MIME("m3u8", "application/vnd.apple.mpegurl");
  REGISTER_MIME("m4a", "audio/mp4");
  REGISTER_MIME("m4u", "video/vnd.mpegurl");
  REGISTER_MIME("m4v", "video/x-m4v");
  REGISTER_MIME("ma", "application/mathematica");
  REGISTER_MIME("mads", "application/mads+xml");
  REGISTER_MIME("mag", "application/vnd.ecowin.chart");
  REGISTER_MIME("maker", "application/vnd.framemaker");
  REGISTER_MIME("man", "text/troff");
  REGISTER_MIME("mar", "application/octet-stream");
  REGISTER_MIME("markdown", "text/markdown");
  REGISTER_MIME("mathml", "application/mathml+xml");
  REGISTER_MIME("mb", "application/mathematica");
  REGISTER_MIME("mbk", "application/vnd.mobius.mbk");
  REGISTER_MIME("mbox", "application/mbox");
  REGISTER_MIME("mc1", "application/vnd.medcalcdata");
  REGISTER_MIME("mcd", "application/vnd.mcd");
  REGISTER_MIME("mcurl", "text/vnd.curl.mcurl");
  REGISTER_MIME("md", "text/markdown");
  REGISTER_MIME("mdb", "application/x-msaccess");
  REGISTER_MIME("mdi", "image/vnd.ms-modi");
  REGISTER_MIME("me", "text/troff");
  REGISTER_MIME("mesh", "model/mesh");
  REGISTER_MIME("meta4", "application/metalink4+xml");
  REGISTER_MIME("metalink", "application/metalink+xml");
  REGISTER_MIME("mets", "application/mets+xml");
  REGISTER_MIME("mfm", "application/vnd.mfmp");
  REGISTER_MIME("mft", "application/rpki-manifest");
  REGISTER_MIME("mgp", "application/vnd.osgeo.mapguide.package");
  REGISTER_MIME("mgz", "application/vnd.proteus.magazine");
  REGISTER_MIME("mid", "audio/midi");
  REGISTER_MIME("midi", "audio/midi");
  REGISTER_MIME("mie", "application/x-mie");
  REGISTER_MIME("mif", "application/vnd.mif");
  REGISTER_MIME("mime", "message/rfc822");
  REGISTER_MIME("mj2", "video/mj2");
  REGISTER_MIME("mjp2", "video/mj2");
  REGISTER_MIME("mk3d", "video/x-matroska");
  REGISTER_MIME("mka", "audio/x-matroska");
  REGISTER_MIME("mks", "video/x-matroska");
  REGISTER_MIME("mkv", "video/x-matroska");
  REGISTER_MIME("mlp", "application/vnd.dolby.mlp");
  REGISTER_MIME("mmd", "application/vnd.chipnuts.karaoke-mmd");
  REGISTER_MIME("mmf", "application/vnd.smaf");
  REGISTER_MIME("mmr", "image/vnd.fujixerox.edmics-mmr");
  REGISTER_MIME("mng", "video/x-mng");
  REGISTER_MIME("mny", "application/x-msmoney");
  REGISTER_MIME("mobi", "application/x-mobipocket-ebook");
  REGISTER_MIME("mods", "application/mods+xml");
  REGISTER_MIME("mov", "video/quicktime");
  REGISTER_MIME("movie", "video/x-sgi-movie");
  REGISTER_MIME("mp2", "audio/mpeg");
  REGISTER_MIME("mp21", "application/mp21");
  REGISTER_MIME("mp2a", "audio/mpeg");
  REGISTER_MIME("mp3", "audio/mpeg");
  REGISTER_MIME("mp4", "video/mp4");
  REGISTER_MIME("mp4a", "audio/mp4");
  REGISTER_MIME("mp4s", "application/mp4");
  REGISTER_MIME("mp4v", "video/mp4");
  REGISTER_MIME("mpc", "application/vnd.mophun.certificate");
  REGISTER_MIME("mpe", "video/mpeg");
  REGISTER_MIME("mpeg", "video/mpeg");
  REGISTER_MIME("mpg", "video/mpeg");
  REGISTER_MIME("mpg4", "video/mp4");
  REGISTER_MIME("mpga", "audio/mpeg");
  REGISTER_MIME("mpkg", "application/vnd.apple.installer+xml");
  REGISTER_MIME("mpm", "application/vnd.blueice.multipass");
  REGISTER_MIME("mpn", "application/vnd.mophun.application");
  REGISTER_MIME("mpp", "application/vnd.ms-project");
  REGISTER_MIME("mpt", "application/vnd.ms-project");
  REGISTER_MIME("mpy", "application/vnd.ibm.minipay");
  REGISTER_MIME("mqy", "application/vnd.mobius.mqy");
  REGISTER_MIME("mrc", "application/marc");
  REGISTER_MIME("mrcx", "application/marcxml+xml");
  REGISTER_MIME("ms", "text/troff");
  REGISTER_MIME("mscml", "application/mediaservercontrol+xml");
  REGISTER_MIME("mseed", "application/vnd.fdsn.mseed");
  REGISTER_MIME("mseq", "application/vnd.mseq");
  REGISTER_MIME("msf", "application/vnd.epson.msf");
  REGISTER_MIME("msh", "model/mesh");
  REGISTER_MIME("msi", "application/x-msdownload");
  REGISTER_MIME("msl", "application/vnd.mobius.msl");
  REGISTER_MIME("msty", "application/vnd.muvee.style");
  REGISTER_MIME("mts", "model/vnd.mts");
  REGISTER_MIME("mus", "application/vnd.musician");
  REGISTER_MIME("musicxml", "application/vnd.recordare.musicxml+xml");
  REGISTER_MIME("mvb", "application/x-msmediaview");
  REGISTER_MIME("mwf", "application/vnd.mfer");
  REGISTER_MIME("mxf", "application/mxf");
  REGISTER_MIME("mxl", "application/vnd.recordare.musicxml");
  REGISTER_MIME("mxml", "application/xv+xml");
  REGISTER_MIME("mxs", "application/vnd.triscape.mxs");
  REGISTER_MIME("mxu", "video/vnd.mpegurl");
  REGISTER_MIME("n-gage", "application/vnd.nokia.n-gage.symbian.install");
  REGISTER_MIME("n3", "text/n3");
  REGISTER_MIME("nb", "application/mathematica");
  REGISTER_MIME("nbp", "application/vnd.wolfram.player");
  REGISTER_MIME("nc", "application/x-netcdf");
  REGISTER_MIME("ncx", "application/x-dtbncx+xml");
  REGISTER_MIME("nfo", "text/x-nfo");
  REGISTER_MIME("ngdat", "application/vnd.nokia.n-gage.data");
  REGISTER_MIME("nitf", "application/vnd.nitf");
  REGISTER_MIME("nlu", "application/vnd.neurolanguage.nlu");
  REGISTER_MIME("nml", "application/vnd.enliven");
  REGISTER_MIME("nnd", "application/vnd.noblenet-directory");
  REGISTER_MIME("nns", "application/vnd.noblenet-sealer");
  REGISTER_MIME("nnw", "application/vnd.noblenet-web");
  REGISTER_MIME("npx", "image/vnd.net-fpx");
  REGISTER_MIME("nsc", "application/x-conference");
  REGISTER_MIME("nsf", "application/vnd.lotus-notes");
  REGISTER_MIME("ntf", "application/vnd.nitf");
  REGISTER_MIME("nzb", "application/x-nzb");
  REGISTER_MIME("oa2", "application/vnd.fujitsu.oasys2");
  REGISTER_MIME("oa3", "application/vnd.fujitsu.oasys3");
  REGISTER_MIME("oas", "application/vnd.fujitsu.oasys");
  REGISTER_MIME("obd", "application/x-msbinder");
  REGISTER_MIME("obj", "application/x-tgif");
  REGISTER_MIME("oda", "application/oda");
  REGISTER_MIME("odb", "application/vnd.oasis.opendocument.database");
  REGISTER_MIME("odc", "application/vnd.oasis.opendocument.chart");
  REGISTER_MIME("odf", "application/vnd.oasis.opendocument.formula");
  REGISTER_MIME("odft", "application/vnd.oasis.opendocument.formula-template");
  REGISTER_MIME("odg", "application/vnd.oasis.opendocument.graphics");
  REGISTER_MIME("odi", "application/vnd.oasis.opendocument.image");
  REGISTER_MIME("odm", "application/vnd.oasis.opendocument.text-master");
  REGISTER_MIME("odp", "application/vnd.oasis.opendocument.presentation");
  REGISTER_MIME("ods", "application/vnd.oasis.opendocument.spreadsheet");
  REGISTER_MIME("odt", "application/vnd.oasis.opendocument.text");
  REGISTER_MIME("oga", "audio/ogg");
  REGISTER_MIME("ogg", "audio/ogg");
  REGISTER_MIME("ogv", "video/ogg");
  REGISTER_MIME("ogx", "application/ogg");
  REGISTER_MIME("omdoc", "application/omdoc+xml");
  REGISTER_MIME("onepkg", "application/onenote");
  REGISTER_MIME("onetmp", "application/onenote");
  REGISTER_MIME("onetoc", "application/onenote");
  REGISTER_MIME("onetoc2", "application/onenote");
  REGISTER_MIME("opf", "application/oebps-package+xml");
  REGISTER_MIME("opml", "text/x-opml");
  REGISTER_MIME("oprc", "application/vnd.palm");
  REGISTER_MIME("org", "application/vnd.lotus-organizer");
  REGISTER_MIME("osf", "application/vnd.yamaha.openscoreformat");
  REGISTER_MIME("osfpvg", "application/vnd.yamaha.openscoreformat.osfpvg+xml");
  REGISTER_MIME("otc", "application/vnd.oasis.opendocument.chart-template");
  REGISTER_MIME("otf", "application/x-font-otf");
  REGISTER_MIME("otg", "application/vnd.oasis.opendocument.graphics-template");
  REGISTER_MIME("oth", "application/vnd.oasis.opendocument.text-web");
  REGISTER_MIME("oti", "application/vnd.oasis.opendocument.image-template");
  REGISTER_MIME("otp",
                "application/vnd.oasis.opendocument.presentation-template");
  REGISTER_MIME("ots",
                "application/vnd.oasis.opendocument.spreadsheet-template");
  REGISTER_MIME("ott", "application/vnd.oasis.opendocument.text-template");
  REGISTER_MIME("oxps", "application/oxps");
  REGISTER_MIME("oxt", "application/vnd.openofficeorg.extension");
  REGISTER_MIME("p", "text/x-pascal");
  REGISTER_MIME("p10", "application/pkcs10");
  REGISTER_MIME("p12", "application/x-pkcs12");
  REGISTER_MIME("p7b", "application/x-pkcs7-certificates");
  REGISTER_MIME("p7c", "application/pkcs7-mime");
  REGISTER_MIME("p7m", "application/pkcs7-mime");
  REGISTER_MIME("p7r", "application/x-pkcs7-certreqresp");
  REGISTER_MIME("p7s", "application/pkcs7-signature");
  REGISTER_MIME("p8", "application/pkcs8");
  REGISTER_MIME("pas", "text/x-pascal");
  REGISTER_MIME("paw", "application/vnd.pawaafile");
  REGISTER_MIME("pbd", "application/vnd.powerbuilder6");
  REGISTER_MIME("pbm", "image/x-portable-bitmap");
  REGISTER_MIME("pcap", "application/vnd.tcpdump.pcap");
  REGISTER_MIME("pcf", "application/x-font-pcf");
  REGISTER_MIME("pcl", "application/vnd.hp-pcl");
  REGISTER_MIME("pclxl", "application/vnd.hp-pclxl");
  REGISTER_MIME("pct", "image/x-pict");
  REGISTER_MIME("pcurl", "application/vnd.curl.pcurl");
  REGISTER_MIME("pcx", "image/x-pcx");
  REGISTER_MIME("pdb", "application/vnd.palm");
  REGISTER_MIME("pdf", "application/pdf");
  REGISTER_MIME("pfa", "application/x-font-type1");
  REGISTER_MIME("pfb", "application/x-font-type1");
  REGISTER_MIME("pfm", "application/x-font-type1");
  REGISTER_MIME("pfr", "application/font-tdpfr");
  REGISTER_MIME("pfx", "application/x-pkcs12");
  REGISTER_MIME("pgm", "image/x-portable-graymap");
  REGISTER_MIME("pgn", "application/x-chess-pgn");
  REGISTER_MIME("pgp", "application/pgp-encrypted");
  REGISTER_MIME("pic", "image/x-pict");
  REGISTER_MIME("pkg", "application/octet-stream");
  REGISTER_MIME("pki", "application/pkixcmp");
  REGISTER_MIME("pkipath", "application/pkix-pkipath");
  REGISTER_MIME("plb", "application/vnd.3gpp.pic-bw-large");
  REGISTER_MIME("plc", "application/vnd.mobius.plc");
  REGISTER_MIME("plf", "application/vnd.pocketlearn");
  REGISTER_MIME("pls", "application/pls+xml");
  REGISTER_MIME("pml", "application/vnd.ctc-posml");
  REGISTER_MIME("png", "image/png");
  REGISTER_MIME("pnm", "image/x-portable-anymap");
  REGISTER_MIME("portpkg", "application/vnd.macports.portpkg");
  REGISTER_MIME("pot", "application/vnd.ms-powerpoint");
  REGISTER_MIME("potm",
                "application/vnd.ms-powerpoint.template.macroenabled.12");
  REGISTER_MIME(
      "potx",
      "application/vnd.openxmlformats-officedocument.presentationml.template");
  REGISTER_MIME("ppam", "application/vnd.ms-powerpoint.addin.macroenabled.12");
  REGISTER_MIME("ppd", "application/vnd.cups-ppd");
  REGISTER_MIME("ppm", "image/x-portable-pixmap");
  REGISTER_MIME("pps", "application/vnd.ms-powerpoint");
  REGISTER_MIME("ppsm",
                "application/vnd.ms-powerpoint.slideshow.macroenabled.12");
  REGISTER_MIME(
      "ppsx",
      "application/vnd.openxmlformats-officedocument.presentationml.slideshow");
  REGISTER_MIME("ppt", "application/vnd.ms-powerpoint");
  REGISTER_MIME("pptm",
                "application/vnd.ms-powerpoint.presentation.macroenabled.12");
  REGISTER_MIME("pqa", "application/vnd.palm");
  REGISTER_MIME("prc", "application/x-mobipocket-ebook");
  REGISTER_MIME("pre", "application/vnd.lotus-freelance");
  REGISTER_MIME("prf", "application/pics-rules");
  REGISTER_MIME("ps", "application/postscript");
  REGISTER_MIME("psb", "application/vnd.3gpp.pic-bw-small");
  REGISTER_MIME("psd", "image/vnd.adobe.photoshop");
  REGISTER_MIME("psf", "application/x-font-linux-psf");
  REGISTER_MIME("pskcxml", "application/pskc+xml");
  REGISTER_MIME("ptid", "application/vnd.pvi.ptid1");
  REGISTER_MIME("pub", "application/x-mspublisher");
  REGISTER_MIME("pvb", "application/vnd.3gpp.pic-bw-var");
  REGISTER_MIME("pwn", "application/vnd.3m.post-it-notes");
  REGISTER_MIME("pya", "audio/vnd.ms-playready.media.pya");
  REGISTER_MIME("pyv", "video/vnd.ms-playready.media.pyv");
  REGISTER_MIME("qam", "application/vnd.epson.quickanime");
  REGISTER_MIME("qbo", "application/vnd.intu.qbo");
  REGISTER_MIME("qfx", "application/vnd.intu.qfx");
  REGISTER_MIME("qps", "application/vnd.publishare-delta-tree");
  REGISTER_MIME("qt", "video/quicktime");
  REGISTER_MIME("qwd", "application/vnd.quark.quarkxpress");
  REGISTER_MIME("qwt", "application/vnd.quark.quarkxpress");
  REGISTER_MIME("qxb", "application/vnd.quark.quarkxpress");
  REGISTER_MIME("qxd", "application/vnd.quark.quarkxpress");
  REGISTER_MIME("qxl", "application/vnd.quark.quarkxpress");
  REGISTER_MIME("qxt", "application/vnd.quark.quarkxpress");
  REGISTER_MIME("ra", "audio/x-pn-realaudio");
  REGISTER_MIME("ram", "audio/x-pn-realaudio");
  REGISTER_MIME("rar", "application/x-rar-compressed");
  REGISTER_MIME("ras", "image/x-cmu-raster");
  REGISTER_MIME("rcprofile", "application/vnd.ipunplugged.rcprofile");
  REGISTER_MIME("rdf", "application/rdf+xml");
  REGISTER_MIME("rdz", "application/vnd.data-vision.rdz");
  REGISTER_MIME("rep", "application/vnd.businessobjects");
  REGISTER_MIME("res", "application/x-dtbresource+xml");
  REGISTER_MIME("rgb", "image/x-rgb");
  REGISTER_MIME("rif", "application/reginfo+xml");
  REGISTER_MIME("rip", "audio/vnd.rip");
  REGISTER_MIME("ris", "application/x-research-info-systems");
  REGISTER_MIME("rl", "application/resource-lists+xml");
  REGISTER_MIME("rlc", "image/vnd.fujixerox.edmics-rlc");
  REGISTER_MIME("rld", "application/resource-lists-diff+xml");
  REGISTER_MIME("rm", "application/vnd.rn-realmedia");
  REGISTER_MIME("rmi", "audio/midi");
  REGISTER_MIME("rmp", "audio/x-pn-realaudio-plugin");
  REGISTER_MIME("rms", "application/vnd.jcp.javame.midlet-rms");
  REGISTER_MIME("rmvb", "application/vnd.rn-realmedia-vbr");
  REGISTER_MIME("rnc", "application/relax-ng-compact-syntax");
  REGISTER_MIME("roa", "application/rpki-roa");
  REGISTER_MIME("roff", "text/troff");
  REGISTER_MIME("rp9", "application/vnd.cloanto.rp9");
  REGISTER_MIME("rpss", "application/vnd.nokia.radio-presets");
  REGISTER_MIME("rpst", "application/vnd.nokia.radio-preset");
  REGISTER_MIME("rq", "application/sparql-query");
  REGISTER_MIME("rs", "application/rls-services+xml");
  REGISTER_MIME("rsd", "application/rsd+xml");
  REGISTER_MIME("rss", "application/rss+xml");
  REGISTER_MIME("rtf", "application/rtf");
  REGISTER_MIME("rtx", "text/richtext");
  REGISTER_MIME("s", "text/x-asm");
  REGISTER_MIME("s3m", "audio/s3m");
  REGISTER_MIME("saf", "application/vnd.yamaha.smaf-audio");
  REGISTER_MIME("sbml", "application/sbml+xml");
  REGISTER_MIME("sc", "application/vnd.ibm.secure-container");
  REGISTER_MIME("scd", "application/x-msschedule");
  REGISTER_MIME("scm", "application/vnd.lotus-screencam");
  REGISTER_MIME("scq", "application/scvp-cv-request");
  REGISTER_MIME("scs", "application/scvp-cv-response");
  REGISTER_MIME("scurl", "text/vnd.curl.scurl");
  REGISTER_MIME("sda", "application/vnd.stardivision.draw");
  REGISTER_MIME("sdc", "application/vnd.stardivision.calc");
  REGISTER_MIME("sdd", "application/vnd.stardivision.impress");
  REGISTER_MIME("sdkd", "application/vnd.solent.sdkm+xml");
  REGISTER_MIME("sdkm", "application/vnd.solent.sdkm+xml");
  REGISTER_MIME("sdp", "application/sdp");
  REGISTER_MIME("sdw", "application/vnd.stardivision.writer");
  REGISTER_MIME("see", "application/vnd.seemail");
  REGISTER_MIME("seed", "application/vnd.fdsn.seed");
  REGISTER_MIME("sema", "application/vnd.sema");
  REGISTER_MIME("semd", "application/vnd.semd");
  REGISTER_MIME("semf", "application/vnd.semf");
  REGISTER_MIME("ser", "application/java-serialized-object");
  REGISTER_MIME("setpay", "application/set-payment-initiation");
  REGISTER_MIME("setreg", "application/set-registration-initiation");
  REGISTER_MIME("sfd-hdstx", "application/vnd.hydrostatix.sof-data");
  REGISTER_MIME("sfs", "application/vnd.spotfire.sfs");
  REGISTER_MIME("sfv", "text/x-sfv");
  REGISTER_MIME("sgi", "image/sgi");
  REGISTER_MIME("sgl", "application/vnd.stardivision.writer-global");
  REGISTER_MIME("sgm", "text/sgml");
  REGISTER_MIME("sgml", "text/sgml");
  REGISTER_MIME("sh", "application/x-sh");
  REGISTER_MIME("shar", "application/x-shar");
  REGISTER_MIME("shf", "application/shf+xml");
  REGISTER_MIME("sid", "image/x-mrsid-image");
  REGISTER_MIME("sig", "application/pgp-signature");
  REGISTER_MIME("sil", "audio/silk");
  REGISTER_MIME("silo", "model/mesh");
  REGISTER_MIME("sis", "application/vnd.symbian.install");
  REGISTER_MIME("sisx", "application/vnd.symbian.install");
  REGISTER_MIME("sit", "application/x-stuffit");
  REGISTER_MIME("sitx", "application/x-stuffitx");
  REGISTER_MIME("skd", "application/vnd.koan");
  REGISTER_MIME("skm", "application/vnd.koan");
  REGISTER_MIME("skp", "application/vnd.koan");
  REGISTER_MIME("skt", "application/vnd.koan");
  REGISTER_MIME("sldm", "application/vnd.ms-powerpoint.slide.macroenabled.12");
  REGISTER_MIME(
      "sldx",
      "application/vnd.openxmlformats-officedocument.presentationml.slide");
  REGISTER_MIME("slt", "application/vnd.epson.salt");
  REGISTER_MIME("sm", "application/vnd.stepmania.stepchart");
  REGISTER_MIME("smf", "application/vnd.stardivision.math");
  REGISTER_MIME("smi", "application/smil+xml");
  REGISTER_MIME("smil", "application/smil+xml");
  REGISTER_MIME("smv", "video/x-smv");
  REGISTER_MIME("smzip", "application/vnd.stepmania.package");
  REGISTER_MIME("snd", "audio/basic");
  REGISTER_MIME("snf", "application/x-font-snf");
  REGISTER_MIME("so", "application/octet-stream");
  REGISTER_MIME("spc", "application/x-pkcs7-certificates");
  REGISTER_MIME("spf", "application/vnd.yamaha.smaf-phrase");
  REGISTER_MIME("spl", "application/x-futuresplash");
  REGISTER_MIME("spot", "text/vnd.in3d.spot");
  REGISTER_MIME("spp", "application/scvp-vp-response");
  REGISTER_MIME("spq", "application/scvp-vp-request");
  REGISTER_MIME("spx", "audio/ogg");
  REGISTER_MIME("sql", "application/x-sql");
  REGISTER_MIME("src", "application/x-wais-source");
  REGISTER_MIME("srt", "application/x-subrip");
  REGISTER_MIME("sru", "application/sru+xml");
  REGISTER_MIME("srx", "application/sparql-results+xml");
  REGISTER_MIME("ssdl", "application/ssdl+xml");
  REGISTER_MIME("sse", "application/vnd.kodak-descriptor");
  REGISTER_MIME("ssf", "application/vnd.epson.ssf");
  REGISTER_MIME("ssml", "application/ssml+xml");
  REGISTER_MIME("st", "application/vnd.sailingtracker.track");
  REGISTER_MIME("stc", "application/vnd.sun.xml.calc.template");
  REGISTER_MIME("std", "application/vnd.sun.xml.draw.template");
  REGISTER_MIME("stf", "application/vnd.wt.stf");
  REGISTER_MIME("sti", "application/vnd.sun.xml.impress.template");
  REGISTER_MIME("stk", "application/hyperstudio");
  REGISTER_MIME("stl", "application/vnd.ms-pki.stl");
  REGISTER_MIME("str", "application/vnd.pg.format");
  REGISTER_MIME("stw", "application/vnd.sun.xml.writer.template");
  REGISTER_MIME("sub", "text/vnd.dvb.subtitle");
  REGISTER_MIME("sus", "application/vnd.sus-calendar");
  REGISTER_MIME("susp", "application/vnd.sus-calendar");
  REGISTER_MIME("sv4cpio", "application/x-sv4cpio");
  REGISTER_MIME("sv4crc", "application/x-sv4crc");
  REGISTER_MIME("svc", "application/vnd.dvb.service");
  REGISTER_MIME("svd", "application/vnd.svd");
  REGISTER_MIME("svg", "image/svg+xml");
  REGISTER_MIME("svgz", "image/svg+xml");
  REGISTER_MIME("swa", "application/x-director");
  REGISTER_MIME("swf", "application/x-shockwave-flash");
  REGISTER_MIME("swi", "application/vnd.aristanetworks.swi");
  REGISTER_MIME("sxc", "application/vnd.sun.xml.calc");
  REGISTER_MIME("sxd", "application/vnd.sun.xml.draw");
  REGISTER_MIME("sxg", "application/vnd.sun.xml.writer.global");
  REGISTER_MIME("sxi", "application/vnd.sun.xml.impress");
  REGISTER_MIME("sxm", "application/vnd.sun.xml.math");
  REGISTER_MIME("sxw", "application/vnd.sun.xml.writer");
  REGISTER_MIME("t", "text/troff");
  REGISTER_MIME("t3", "application/x-t3vm-image");
  REGISTER_MIME("taglet", "application/vnd.mynfc");
  REGISTER_MIME("tao", "application/vnd.tao.intent-module-archive");
  REGISTER_MIME("tar", "application/x-tar");
  REGISTER_MIME("tcap", "application/vnd.3gpp2.tcap");
  REGISTER_MIME("tcl", "application/x-tcl");
  REGISTER_MIME("teacher", "application/vnd.smart.teacher");
  REGISTER_MIME("tei", "application/tei+xml");
  REGISTER_MIME("teicorpus", "application/tei+xml");
  REGISTER_MIME("tex", "application/x-tex");
  REGISTER_MIME("texi", "application/x-texinfo");
  REGISTER_MIME("texinfo", "application/x-texinfo");
  REGISTER_MIME("text", "text/plain");
  REGISTER_MIME("tfi", "application/thraud+xml");
  REGISTER_MIME("tfm", "application/x-tex-tfm");
  REGISTER_MIME("tga", "image/x-tga");
  REGISTER_MIME("thmx", "application/vnd.ms-officetheme");
  REGISTER_MIME("tif", "image/tiff");
  REGISTER_MIME("tiff", "image/tiff");
  REGISTER_MIME("tmo", "application/vnd.tmobile-livetv");
  REGISTER_MIME("torrent", "application/x-bittorrent");
  REGISTER_MIME("tpl", "application/vnd.groove-tool-template");
  REGISTER_MIME("tpt", "application/vnd.trid.tpt");
  REGISTER_MIME("tr", "text/troff");
  REGISTER_MIME("tra", "application/vnd.trueapp");
  REGISTER_MIME("trm", "application/x-msterminal");
  REGISTER_MIME("tsd", "application/timestamped-data");
  REGISTER_MIME("tsv", "text/tab-separated-values");
  REGISTER_MIME("ttc", "application/x-font-ttf");
  REGISTER_MIME("ttf", "application/x-font-ttf");
  REGISTER_MIME("ttl", "text/turtle");
  REGISTER_MIME("twd", "application/vnd.simtech-mindmapper");
  REGISTER_MIME("twds", "application/vnd.simtech-mindmapper");
  REGISTER_MIME("txd", "application/vnd.genomatix.tuxedo");
  REGISTER_MIME("txf", "application/vnd.mobius.txf");
  REGISTER_MIME("u32", "application/x-authorware-bin");
  REGISTER_MIME("udeb", "application/x-debian-package");
  REGISTER_MIME("ufd", "application/vnd.ufdl");
  REGISTER_MIME("ufdl", "application/vnd.ufdl");
  REGISTER_MIME("ulx", "application/x-glulx");
  REGISTER_MIME("umj", "application/vnd.umajin");
  REGISTER_MIME("unityweb", "application/vnd.unity");
  REGISTER_MIME("uoml", "application/vnd.uoml+xml");
  REGISTER_MIME("uri", "text/uri-list");
  REGISTER_MIME("uris", "text/uri-list");
  REGISTER_MIME("urls", "text/uri-list");
  REGISTER_MIME("ustar", "application/x-ustar");
  REGISTER_MIME("utz", "application/vnd.uiq.theme");
  REGISTER_MIME("uu", "text/x-uuencode");
  REGISTER_MIME("uva", "audio/vnd.dece.audio");
  REGISTER_MIME("uvd", "application/vnd.dece.data");
  REGISTER_MIME("uvf", "application/vnd.dece.data");
  REGISTER_MIME("uvg", "image/vnd.dece.graphic");
  REGISTER_MIME("uvh", "video/vnd.dece.hd");
  REGISTER_MIME("uvi", "image/vnd.dece.graphic");
  REGISTER_MIME("uvm", "video/vnd.dece.mobile");
  REGISTER_MIME("uvp", "video/vnd.dece.pd");
  REGISTER_MIME("uvs", "video/vnd.dece.sd");
  REGISTER_MIME("uvt", "application/vnd.dece.ttml+xml");
  REGISTER_MIME("uvu", "video/vnd.uvvu.mp4");
  REGISTER_MIME("uvv", "video/vnd.dece.video");
  REGISTER_MIME("uvva", "audio/vnd.dece.audio");
  REGISTER_MIME("uvvd", "application/vnd.dece.data");
  REGISTER_MIME("uvvf", "application/vnd.dece.data");
  REGISTER_MIME("uvvg", "image/vnd.dece.graphic");
  REGISTER_MIME("uvvh", "video/vnd.dece.hd");
  REGISTER_MIME("uvvi", "image/vnd.dece.graphic");
  REGISTER_MIME("uvvm", "video/vnd.dece.mobile");
  REGISTER_MIME("uvvp", "video/vnd.dece.pd");
  REGISTER_MIME("uvvs", "video/vnd.dece.sd");
  REGISTER_MIME("uvvt", "application/vnd.dece.ttml+xml");
  REGISTER_MIME("uvvu", "video/vnd.uvvu.mp4");
  REGISTER_MIME("uvvv", "video/vnd.dece.video");
  REGISTER_MIME("uvvx", "application/vnd.dece.unspecified");
  REGISTER_MIME("uvvz", "application/vnd.dece.zip");
  REGISTER_MIME("uvx", "application/vnd.dece.unspecified");
  REGISTER_MIME("uvz", "application/vnd.dece.zip");
  REGISTER_MIME("vcard", "text/vcard");
  REGISTER_MIME("vcd", "application/x-cdlink");
  REGISTER_MIME("vcf", "text/x-vcard");
  REGISTER_MIME("vcg", "application/vnd.groove-vcard");
  REGISTER_MIME("vcs", "text/x-vcalendar");
  REGISTER_MIME("vcx", "application/vnd.vcx");
  REGISTER_MIME("vis", "application/vnd.visionary");
  REGISTER_MIME("viv", "video/vnd.vivo");
  REGISTER_MIME("vob", "video/x-ms-vob");
  REGISTER_MIME("vor", "application/vnd.stardivision.writer");
  REGISTER_MIME("vox", "application/x-authorware-bin");
  REGISTER_MIME("vrml", "model/vrml");
  REGISTER_MIME("vsd", "application/vnd.visio");
  REGISTER_MIME("vsf", "application/vnd.vsf");
  REGISTER_MIME("vss", "application/vnd.visio");
  REGISTER_MIME("vst", "application/vnd.visio");
  REGISTER_MIME("vsw", "application/vnd.visio");
  REGISTER_MIME("vtu", "model/vnd.vtu");
  REGISTER_MIME("vxml", "application/voicexml+xml");
  REGISTER_MIME("w3d", "application/x-director");
  REGISTER_MIME("wad", "application/x-doom");
  REGISTER_MIME("wav", "audio/x-wav");
  REGISTER_MIME("wax", "audio/x-ms-wax");
  REGISTER_MIME("wbmp", "image/vnd.wap.wbmp");
  REGISTER_MIME("wbs", "application/vnd.criticaltools.wbs+xml");
  REGISTER_MIME("wbxml", "application/vnd.wap.wbxml");
  REGISTER_MIME("wcm", "application/vnd.ms-works");
  REGISTER_MIME("wdb", "application/vnd.ms-works");
  REGISTER_MIME("wdp", "image/vnd.ms-photo");
  REGISTER_MIME("weba", "audio/webm");
  REGISTER_MIME("webm", "video/webm");
  REGISTER_MIME("webp", "image/webp");
  REGISTER_MIME("wg", "application/vnd.pmi.widget");
  REGISTER_MIME("wgt", "application/widget");
  REGISTER_MIME("wks", "application/vnd.ms-works");
  REGISTER_MIME("wm", "video/x-ms-wm");
  REGISTER_MIME("wma", "audio/x-ms-wma");
  REGISTER_MIME("wmd", "application/x-ms-wmd");
  REGISTER_MIME("wmf", "application/x-msmetafile");
  REGISTER_MIME("wml", "text/vnd.wap.wml");
  REGISTER_MIME("wmlc", "application/vnd.wap.wmlc");
  REGISTER_MIME("wmls", "text/vnd.wap.wmlscript");
  REGISTER_MIME("wmlsc", "application/vnd.wap.wmlscriptc");
  REGISTER_MIME("wmv", "video/x-ms-wmv");
  REGISTER_MIME("wmx", "video/x-ms-wmx");
  REGISTER_MIME("wmz", "application/x-ms-wmz");
  // REGISTER_MIME("wmz", "application/x-msmetafile");
  REGISTER_MIME("woff", "application/font-woff");
  REGISTER_MIME("wpd", "application/vnd.wordperfect");
  REGISTER_MIME("wpl", "application/vnd.ms-wpl");
  REGISTER_MIME("wps", "application/vnd.ms-works");
  REGISTER_MIME("wqd", "application/vnd.wqd");
  REGISTER_MIME("wri", "application/x-mswrite");
  REGISTER_MIME("wrl", "model/vrml");
  REGISTER_MIME("wsdl", "application/wsdl+xml");
  REGISTER_MIME("wspolicy", "application/wspolicy+xml");
  REGISTER_MIME("wtb", "application/vnd.webturbo");
  REGISTER_MIME("wvx", "video/x-ms-wvx");
  REGISTER_MIME("x32", "application/x-authorware-bin");
  REGISTER_MIME("x3d", "model/x3d+xml");
  REGISTER_MIME("x3db", "model/x3d+binary");
  REGISTER_MIME("x3dbz", "model/x3d+binary");
  REGISTER_MIME("x3dv", "model/x3d+vrml");
  REGISTER_MIME("x3dvz", "model/x3d+vrml");
  REGISTER_MIME("x3dz", "model/x3d+xml");
  REGISTER_MIME("xaml", "application/xaml+xml");
  REGISTER_MIME("xap", "application/x-silverlight-app");
  REGISTER_MIME("xar", "application/vnd.xara");
  REGISTER_MIME("xbap", "application/x-ms-xbap");
  REGISTER_MIME("xbd", "application/vnd.fujixerox.docuworks.binder");
  REGISTER_MIME("xbm", "image/x-xbitmap");
  REGISTER_MIME("xdf", "application/xcap-diff+xml");
  REGISTER_MIME("xdm", "application/vnd.syncml.dm+xml");
  REGISTER_MIME("xdp", "application/vnd.adobe.xdp+xml");
  REGISTER_MIME("xdssc", "application/dssc+xml");
  REGISTER_MIME("xdw", "application/vnd.fujixerox.docuworks");
  REGISTER_MIME("xenc", "application/xenc+xml");
  REGISTER_MIME("xer", "application/patch-ops-error+xml");
  REGISTER_MIME("xfdf", "application/vnd.adobe.xfdf");
  REGISTER_MIME("xfdl", "application/vnd.xfdl");
  REGISTER_MIME("xht", "application/xhtml+xml");
  REGISTER_MIME("xhtml", "application/xhtml+xml");
  REGISTER_MIME("xhvml", "application/xv+xml");
  REGISTER_MIME("xif", "image/vnd.xiff");
  REGISTER_MIME("xla", "application/vnd.ms-excel");
  REGISTER_MIME("xlam", "application/vnd.ms-excel.addin.macroenabled.12");
  REGISTER_MIME("xlc", "application/vnd.ms-excel");
  REGISTER_MIME("xlf", "application/x-xliff+xml");
  REGISTER_MIME("xlm", "application/vnd.ms-excel");
  REGISTER_MIME("xls", "application/vnd.ms-excel");
  REGISTER_MIME("xlsb",
                "application/vnd.ms-excel.sheet.binary.macroenabled.12");
  REGISTER_MIME("xlsm", "application/vnd.ms-excel.sheet.macroenabled.12");
  REGISTER_MIME(
      "xlsx",
      "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
  REGISTER_MIME("xlt", "application/vnd.ms-excel");
  REGISTER_MIME("xltm", "application/vnd.ms-excel.template.macroenabled.12");
  REGISTER_MIME(
      "xltx",
      "application/vnd.openxmlformats-officedocument.spreadsheetml.template");
  REGISTER_MIME("xlw", "application/vnd.ms-excel");
  REGISTER_MIME("xm", "audio/xm");
  REGISTER_MIME("xml", "application/xml");
  REGISTER_MIME("xo", "application/vnd.olpc-sugar");
  REGISTER_MIME("xop", "application/xop+xml");
  REGISTER_MIME("xpi", "application/x-xpinstall");
  REGISTER_MIME("xpl", "application/xproc+xml");
  REGISTER_MIME("xpm", "image/x-xpixmap");
  REGISTER_MIME("xpr", "application/vnd.is-xpr");
  REGISTER_MIME("xps", "application/vnd.ms-xpsdocument");
  REGISTER_MIME("xpw", "application/vnd.intercon.formnet");
  REGISTER_MIME("xpx", "application/vnd.intercon.formnet");
  REGISTER_MIME("xsl", "application/xml");
  REGISTER_MIME("xslt", "application/xslt+xml");
  REGISTER_MIME("xsm", "application/vnd.syncml+xml");
  REGISTER_MIME("xspf", "application/xspf+xml");
  REGISTER_MIME("xul", "application/vnd.mozilla.xul+xml");
  REGISTER_MIME("xvm", "application/xv+xml");
  REGISTER_MIME("xvml", "application/xv+xml");
  REGISTER_MIME("xwd", "image/x-xwindowdump");
  REGISTER_MIME("xyz", "chemical/x-xyz");
  REGISTER_MIME("xz", "application/x-xz");
  REGISTER_MIME("yang", "application/yang");
  REGISTER_MIME("yin", "application/yin+xml");
  REGISTER_MIME("z1", "application/x-zmachine");
  REGISTER_MIME("z2", "application/x-zmachine");
  REGISTER_MIME("z3", "application/x-zmachine");
  REGISTER_MIME("z4", "application/x-zmachine");
  REGISTER_MIME("z5", "application/x-zmachine");
  REGISTER_MIME("z6", "application/x-zmachine");
  REGISTER_MIME("z7", "application/x-zmachine");
  REGISTER_MIME("z8", "application/x-zmachine");
  REGISTER_MIME("zaz", "application/vnd.zzazz.deck+xml");
  REGISTER_MIME("zip", "application/zip");
  REGISTER_MIME("zir", "application/vnd.zul");
  REGISTER_MIME("zirz", "application/vnd.zul");
  REGISTER_MIME("zmm", "application/vnd.handheld-entertainment+xml");

#endif /* HTTP_MIME_REGISTRY_AUTO > 0*/
#undef REGISTER_MIME

  http_mimetype_stats();
}

#define SERVER_NAME "The WebServer with Thick Thighs (づ｡◕‿‿◕｡)づ"

#define HTTP_VER "1.1"

#define PUBLIC_GROUP "miguel"
#define TEMPLATE_DIR "public/templates"
#define JS_DIR "public/static/js"
#define CSS_DIR "public/static/css"

typedef enum
{
    HTTP_OK,
    HTTP_FORBIDDEN,
    HTTP_NOTFOUND,
    HTTP_METHOD_NOT_ALLOWED,
    HTTP_INTERNAL_SERVER_ERROR,
    HTTP_NOT_IMPLEMENTED,
    HTTP_SERVICE_UNAVAILABLE,
    HTTP_INSUFFICIENT_STORAGE,
}
SERVER_STATUS;

struct error
{
    unsigned status;
    char msg[256];
};

static const struct error SERVER_ERROR_CODE[] = {
    { 200, "The request has succeeded." },
    { 403, "The server understood the request but refuses to authorize it." },
    { 404, "The origin server did not find a current representation for the target resource or is not willing to disclose that one exists." },
    { 405, "The method received in the request-line is known by the origin server but not supported by the target resource." },
    { 500, "The server encountered an unexpected condition that prevented it from fulfilling the request." },
    { 501, "The server does not support the functionality required to fulfill the request." },
    { 503, "The server is currently unable to handle the request due to a temporary overload or scheduled maintenance, which will likely be alleviated after some delay." },
    { 507, "The method could not be performed on the resource because the server is unable to store the representation needed to successfully complete the request." },
};


typedef struct Error
{
    unsigned char error;
    const char *msg;
    struct Error *next;
}
Error_t;

typedef struct
{
    size_t content_lenght;
    unsigned short status;
    TMPL_varlist *TMPL_mainlist;
    char *TMPL_file;
    Error_t *errors;
    char *header;
    char *content;

    sqlite3 *db;
}
Response;


typedef enum
{
    GET,
    POST,
    HEAD,
    UNSUPPORTED,
}
Method;

typedef struct Dict
{
    char *key;
    char *value;
    struct Dict *next;
}
Dict_t;

typedef struct MultiForm
{
    struct Dict *head;
    char *data;
    struct MultiForm *next;
}
MultiForm_t;

typedef struct
{
    Method method;
    Dict_t *queries; /* key-value pairs of url address */
    char *route;
    char *version;
    Dict_t *headers;
    char *body;

    /* method is POST */
    Dict_t *form; /* application/x-www-form-urlencoded */
    MultiForm_t *multi; /* multipart/form-data */
}
Request;


int readFileOK(Response *resp);
int writeFileOK(Response *resp);
int execFileOK(Response *resp);

void renderContent(Response *resp);
void addError(Response *resp, unsigned char err);
void freeRequest(Request *req);
void freeResponse(Response *resp);
Request *parseRequest(char *raw);

char *getRequestHeader(Request *req, char *header);
char *getRequestGetField(Request *req, char *field);
char *getRequestPostField(Request *req, char *field);
char *getRouteParam(Request *req, unsigned short pos);
char *setPath(char *fname);
int routeIs(Request *req, char *route);
int routeIsRegEx(Request *req, char *regex);

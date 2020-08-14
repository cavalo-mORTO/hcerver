#define SERVER_NAME "The WebServer with Thick Thighs (づ｡◕‿‿◕｡)づ"


#define HTTP_VER "1.1"
#define HTTP_OK 200
#define HTTP_NOTFOUND 404
#define HTTP_FORBIDDEN 403
#define HTTP_METHOD_NOT_ALLOWED 405
#define HTTP_TIMEOUT 408
#define HTTP_INTERNAL_SERVER_ERROR 500
#define HTTP_NOT_IMPLEMENTED 501
#define HTTP_SERVICE_UNAVAILABLE 503


#define TEMPLATE_DIR "public/templates"
#define JS_DIR "public/static/js"
#define CSS_DIR "public/static/css"


typedef enum
{
    NO_ROUTE,
    NO_FILE,
    FORBIDDEN,
    INTERNAL_ERROR,
    METHOD_NOT_ALLOWED,
}
SERVER_ERROR;

static const char SERVER_ERROR_MSG[][128] = {
    "Route not found!",
    "The requested page couldn't be found!",
    "The requested page is forbidden!",
    "Internal server error!",
    "Method not allowed!",
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
    unsigned int status;
    TMPL_varlist *TMPL_mainlist;
    char *TMPL_file;
    Error_t *errors;
    char *header;
    char *content;
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

typedef struct
{
    Method method;
    Dict_t *queries; /* key-value pairs of url address */
    char *route;
    char *version;
    Dict_t *headers;
    char *body;
    Dict_t *posts; /* only is filled is request method is POST */
}
Request;


/* utils.c */
int max(int a, int b);
int min(int a, int b);
void readFileOK(Response *resp);
void writeFileOK(Response *resp);

/* server.c */
void renderContent(Response *resp);
void addError(Response *resp, unsigned char err);
void freeRequest(Request *req);
void freeResponse(Response *resp);
Request *parseRequest(char *raw);


char *getRequestUrlArg(Request *req, char *argToFind);
char *getRequestPostArg(Request *req, char *argToFind);
int regexMatch(char *regexStr, char *matchStr);
char *getRouteParam(Request *req, unsigned int pos);
char *setPath(char *fname);
int routeIs(Request *req, char *route);
int routeIsRegEx(Request *req, char *regex);

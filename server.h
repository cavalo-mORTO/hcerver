#define SERVER_NAME "The WebServer with Thick Thighs"


#define HTTP_VER "1.1"
#define HTTP_OK 200
#define HTTP_NOTFOUND 404
#define HTTP_FORBIDDEN 403
#define HTTP_INTERNAL_SERVER_ERROR 500


#define TEMPLATE_DIR "public/templates"
#define JS_DIR "public/static/js"
#define CSS_DIR "public/static/css"


typedef enum
{
    NO_ROUTE,
    NO_FILE,
    FORBIDDEN,
    INTERNAL_ERROR,
}
SERVER_ERROR;

static char *SERVER_ERROR_MSG[] = {
    "Route not found!",
    "The requested page couldn't be found!",
    "The requested page is forbidden!",
    "Internal server error!",
};


typedef struct Error
{
    unsigned short int error;
    char *msg;
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
    Dict_t *queries;
    char *route;
    char *version;
    Dict_t *headers;
    char *body;
}
Request;


/* utils.c */
int max(int a, int b);
int min(int a, int b);
void readFileOK(Response *resp);
void writeFileOK(Response *resp);
void getMimeType(Response *resp, char *mime);
void getHttpMetas(char *buffer, char *metas);
char *setPath(char *fname);

/* server.c */
char *handleRequest(char *raw_request);
void addError(Response *resp, unsigned short int err);

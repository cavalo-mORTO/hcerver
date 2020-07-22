#define HTTP_VER "1.1"
#define HTTP_OK 200
#define HTTP_NOTFOUND 404
#define HTTP_FORBIDDEN 403
#define HTTP_INTERNAL_SERVER_ERROR 500


#define NO_ROUTE 0
#define NO_FILE 1
#define FORBIDDEN 2
#define SERVER_ERROR 3

static const char err_msg[4][100] = {
    "Route not found!",
    "The requested page couldn't be found!",
    "The requested page is forbidden!",
    "Internal server error!",
};

typedef struct Error {
    unsigned short int error;
    char *msg;
    struct Error *next;
} Error;

typedef struct {
    size_t content_lenght;
    unsigned int status;
    TMPL_varlist *TMPL_mainlist;
    char *TMPL_file;
    Error *errors;
    char *header;
    char *content;
} Response;


typedef enum {
    GET,
    POST,
    HEAD,
    UNSUPPORTED,
} Method;

typedef struct Dict {
    char *key;
    char *value;
    struct Dict *next;
} Dict;

typedef struct {
    Method method;
    Dict *queries;
    char *route;
    char *version;
    Dict *headers;
    char *body;
} Request;

// routes.c
void map_route(const Request *req, Response *resp);

// utils.c
int max(int a, int b);
int min(int a, int b);
void check_file(Response *resp);
void get_mime_type(Response *resp, char *mime);
void get_http_metas(char *buffer, char *metas);

// request.c
Request *parse_request(char *raw);

// response.c
char *handle_request(char *raw_request);
void render_content(Response *resp);
char *error();
void add_error(Response *resp, unsigned short int err);
void make_header(Response *resp);

// free.c
void free_request(Request *req);
void free_response(Response *resp);
void free_dict(Dict *d);
void free_error(Error *e);

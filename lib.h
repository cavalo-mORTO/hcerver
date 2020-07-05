#define HTTP_OK 200
#define HTTP_NOTFOUND 404
#define HTTP_FORBIDDEN 403

typedef enum Method {UNSUPPORTED, GET, HEAD} Method;

typedef struct Dict {
    char *key;
    char *value;
    struct Dict *next;
} Dict;

typedef struct Response
{
    size_t content_lenght;
    unsigned int status;
    struct Dict *args;
    char *file;
    char *header;
    char *content;
    char *repr;
} Response;

typedef struct Request {
    enum Method method;
    struct Dict *queries;
    char *route;
    char *version;
    struct Dict *headers;
    char *body;
} Request;

// routes.c
void map_route(Request *req, Response *resp);

// utils.c
int max(int a, int b);
int min(int a, int b);
int check_file(const char *fname);
char *read_file(const char *fname);
char *str_replace(char *input, char *w_old, char *w_new);

// request.c
struct Request *parse_request(char *raw_request);

// response.c
struct Response *handle_request(char *raw_request);
void compose_response(Response *resp);
void render_content(Response *resp);
void append_arg(Response *resp, char *key, char *value);

// free.c
void free_request(Request *req);
void free_response(Response *resp);
void free_dict(Dict *d);

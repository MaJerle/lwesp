

/**
 * \addtogroup      ESP_APP_HTTP_SERVER
 * \{
 *
 * \par             SSI (Server Side Includes) tags support
 *
 * SSI tags are supported on server to include user specific values as replacement of static content.
 *
 * Each tag must start with \ref HTTP_SSI_TAG_START tag and end with \ref HTTP_SSI_TAG_END and tag must not be longer than \ref HTTP_SSI_TAG_MAX_LEN.
 * White spaces are not allowed and "-" character is not allowed in tag name. Example of valid tag is <b>\<\!\-\-\#my_tag\-\-\></b> where name of tag is <b>my_tag</b>.
 *
 * The tag name is later sent to SSI callback function where user can send custom data as tag replacement.
 *
 * \par             CGI (Common Gateway Interface) support
 *
 * CGI support allows you to hook different functions from clients to server.
 *
 *  - CGI paths must be enabled before with callback functions when CGI is triggered
 *  - CGI path must finish with <b>.cgi</b> suffix
 *      - To allow CGI hook, request URI must be in format <b>/folder/subfolder/file.cgi?param1=value1&param2=value2&</b>
 *
 * \par             HTTP server example with CGI and SSI
 *
 * \include         _example_http_server.c
 *
 * \}
 */
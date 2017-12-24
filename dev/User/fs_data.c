#include "main.h"
#include "fs_data.h"

const uint8_t responseData[] = ""
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n"
"\r\n"
"<html>\n"
"   <head>\n"
"       <meta http-equiv=\"Refresh\" content=\"1\" />\n"
"       <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js\"></script>\n"
"       <script src=\"/js/js1.js\" type=\"text/javascript\"></script>\n"
"       <!--<script src=\"/js/js2.js\" type=\"text/javascript\"></script>-->\n"
"       <!--<script src=\"/js/js3.js\" type=\"text/javascript\"></script>-->\n"
"       <!--<script src=\"/js/js4.js\" type=\"text/javascript\"></script>-->\n"
"       <link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style1.css\">\n"
"       <!--<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style2.css\">-->\n"
"       <!--<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style3.css\">-->\n"
"       <!--<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style4.css\">-->\n"
"   </head>\n"
"   <body>\n"
"       <div id=\"maindiv\">\n"
"           <h1>Welcome to web server produced by ESP8266 Wi-Fi module!</h1>\n"
"           <p>\n"
"               Far far away, behind the word mountains, far from the countries Vokalia and Consonantia, there live the blind texts.\n"
"               Separated they live in Bookmarksgrove right at the coast of the Semantics, a large language ocean.\n"
"               A small river named Duden flows by their place and supplies it with the necessary regelialia.\n"
"               It is a paradisematic country, in which roasted parts of sentences fly into your mouth.\n"
"               Even the all-powerful Pointing has no control about the blind texts it is an almost unorthographic life.\n"
"               One day however a small line of blind text by the name of Lorem Ipsum decided to leave for the far World of Grammar.\n"
"               The Big Oxmox advised her not to do so, because there were thousands of bad Commas, wild Question Marks and devious Semikoli, but the Little Blind Text didn’t listen.\n"
"               She packed her seven versalia, put her initial into the belt and made herself on the way.\n"
"               When she reached the first hills of the Italic Mountains, she had a last view back on the skyline of her hometown Bookmarksgrove, the headline of Alphabet Village and the subline of her own road, the Line Lane.\n"
"               Pityful a rethoric question ran over her cheek, then\n"
"           </p>\n"
"           <p>\n"
"               Lorem ipsum dolor sit amet, consectetuer adipiscing elit.\n"
"               Aenean commodo ligula eget dolor. Aenean massa.\n"
"               Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Donec quam felis, ultricies nec, pellentesque eu, pretium quis, sem.\n"
"               Nulla consequat massa quis enim. Donec pede justo, fringilla vel, aliquet nec, vulputate\n"
"           </p>\n"
"           <div>\n"
"               <a href=\"led.cgi?led=green&val=on\"><button>LED On</button></a>\n"
"               <a href=\"led.cgi?led=green&val=off\"><button>LED Off</button></a>\n"
"           </div>\n"
"           <div>\n"
"               <form method=\"post\" enctype=\"multipart/form-data\" action=\"upload.cgi\">\n"
"                   <input type=\"file\" name=\"file1\" />\n"
"                   <input type=\"file\" name=\"file2\" />\n"
"                   <input type=\"file\" name=\"file3\" />\n"
"                   <button type=\"submit\">Upload</button>\n"
"               </form>\n"
"           </div>\n"
"       </div>\n"
"       <footer>\n"
"           <div id=\"footerdiv\">\n"
"               Copyright &copy; 2017. All rights reserved. Webserver is hosted on ESP8266.\n"
"           </div>\n"
"       </footer>\n"
"   </body>\n"
"</html>\n";

const uint8_t responseData_css[] = ""
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/css\r\n"
"Connection: close\r\n"
"\r\n"
"html, body { margin: 0; padding: 0; color: blue; font-family: Arial, Tahoma; }\r\n"
"h1 { font-size: 22px; }\n"
"#maindiv   { margin: 0 auto; width: 1000px; padding: 10px; border: 1px solid #000000; }\n"
"#footerdiv { margin: 0 auto; width: 1000px; padding: 6px 3px; border: 1px solid #000000; font-size: 11px; }\n"
"footer { position: fixed; bottom: 0; width: 100%; background: brown; color: #DDDDDD; }\n"
"";

const uint8_t responseData_js1[] = ""
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/javascript\r\n"
"Connection: close\r\n"
"\r\n"
"jQuery(document).ready(function() {\n"
"   jQuery('body').css('color', 'red');\n"
"})\n";

const uint8_t responseData_js2[] = ""
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/javascript\r\n"
"Connection: close\r\n"
"\r\n"
"document.write(\"TEST STRING\")";

const uint8_t responseData_404[] = ""
"HTTP/1.1 404 Not Found\r\n"
"Content-Type: text/html\r\n"
"Connection: close\r\n"
"\r\n"
"<html>\n"
"   <head>\n"
"       <meta http-equiv=\"Refresh\" content=\"2; url=/\" />\n"
"       <link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style1.css\">\n"
"   </head>\n"
"   <body>\n"
"       <div id=\"maindiv\">\n"
"           <h1>Page not found!</h1>\n"
"       </div>\n"
"       <footer>\n"
"           <div id=\"footerdiv\">\n"
"               Copyright &copy; 2017. All rights reserved. Webserver is hosted on ESP8266.\n"
"           </div>\n"
"       </footer>\n"
"   </body>\n"
"</html>\n";

/**
 * \brief           List of dummy files for output on user request
 */
const fs_file_table_t
files[] = {
    {"/index.html",         responseData,       sizeof(responseData) - 1,       0, 1, 1},
    {"/css/style1.css",     responseData_css,   sizeof(responseData_css) - 1,   0, 1, 1},
    {"/css/style2.css",     responseData_css,   sizeof(responseData_css) - 1,   0, 1, 1},
    {"/css/style3.css",     responseData_css,   sizeof(responseData_css) - 1,   0, 1, 1},
    {"/css/style4.css",     responseData_css,   sizeof(responseData_css) - 1,   0, 1, 1},
    {"/js/js1.js",          responseData_js1,   sizeof(responseData_js1) - 1,   0, 1, 1},
    {"/js/js2.js",          responseData_js2,   sizeof(responseData_js2) - 1,   0, 1, 1},
    {"/js/js3.js",          responseData_js1,   sizeof(responseData_js1) - 1,   0, 1, 1},
    {"/js/js4.js",          responseData_js2,   sizeof(responseData_js2) - 1,   0, 1, 1},
    {"404",                 responseData_404,   sizeof(responseData_404) - 1,   1, 1, 1}
};

/**
 * \brief           Open file from file system
 * \param[in]       path: File path to open
 * \param[in]       is_404: Flag indicating we want 404 error message
 * \return          1 on success or 0 otherwise
 */
uint8_t
fs_data_open_file(fs_file_t* file, const char* path, uint8_t is_404) {
    uint8_t i;
    for (i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        if (
            (is_404 && files[i].is_404) ||
            (!is_404 && path && !strcmp(files[i].path, path))
        ) {
            file->len = files[i].len;
            file->data = (uint8_t *)files[i].data;
            file->is_static = 1;
            return 1;
        }
    }
    return 0;
}

/**
 * \brief           Read part of file
 * \param[in]       file: File handle
 * \param[in]       buff: Pointer to buffer to save read data
 * \param[in]       btr: Number of bytes to read and write to buffer
 * \param[out]      br: Pointer to save number of bytes read to buffer
 */
uint8_t
fs_data_read_file(fs_file_t* file, void* buff, size_t btr, size_t* br) {
    return 1;
}

/**
 * \brief           Close file handle
 * \param[in]       file: Pointer to file handle to close
 */
void
fs_data_close_file(fs_file_t* file) {

}

/**	
 * \file            esp_http_server_fs.c
 * \brief           HTTP server file system wrapper
 */
 
/*
 * Copyright (c) 2018 Tilen Majerle
 *  
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of ESP-AT.
 *
 * Author:          Tilen MAJERLE <tilen@majerle.eu>
 */
#include "esp/apps/esp_http_server.h"
#include "esp/esp_mem.h"

/* Number of opened files in system */
extern uint16_t http_fs_opened_files_cnt;

const uint8_t responseData[] = ""
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "\r\n"
    "<html>\n"
    "   <head>\n"
    "       <title><!--#title--></title>\n"
    "       <meta http-equiv=\"Refresh\" content=\"1\" />\n"
    "       <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js\"></script>\n"
    "       <script src=\"/js/js1.js\" type=\"text/javascript\"></script>\n"
    "       <!--<script src=\"/js/js2.js\" type=\"text/javascript\"></script>-->\n"
    "       <!--<script src=\"/js/js3.js\" type=\"text/javascript\"></script>-->\n"
    "       <!--<script src=\"/js/js4.js\" type=\"text/javascript\"></script>-->\n"
    "       <link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0-beta.2/css/bootstrap.min.css\" integrity=\"sha384-PsH8R72JQ3SOdhVi3uxftmaW6Vc51MKb0q5P2rRUpPvrszuE4W1povHYgTpBfshb\" crossorigin=\"anonymous\" />\n"
    "       <link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style1.css\">\n"
    "       <!--<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style2.css\">-->\n"
    "       <!--<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style3.css\">-->\n"
    "       <!--<link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style4.css\">-->\n"
    "   </head>\n"
    "   <body>\n"
    "       <div id=\"maindiv\">\n"
    "           <h1>Welcome to web server hosted on ESP8266 Wi-Fi module!</h1>\n"
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
    "               <p>LED Status: <b><!--#led_status--></b></p>\n"
    "           </div>\n"
    "           <div>\n"
    "               Available Wi-Fi networks\n"
    "               <p><!--#wifi_list--></p>\n"
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
    "       <link rel=\"stylesheet\" type=\"text/css\" href=\"/css/style1.css\" />\n"
    "       <!-- test -->\n"
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
const http_fs_file_table_t
http_fs_static_files[] = {
    {"/index.html",         responseData,       sizeof(responseData) - 1},
    {"/index.shtml",        responseData,       sizeof(responseData) - 1},
    {"/css/style1.css",     responseData_css,   sizeof(responseData_css) - 1},
    {"/css/style2.css",     responseData_css,   sizeof(responseData_css) - 1},
    {"/css/style3.css",     responseData_css,   sizeof(responseData_css) - 1},
    {"/css/style4.css",     responseData_css,   sizeof(responseData_css) - 1},
    {"/js/js1.js",          responseData_js1,   sizeof(responseData_js1) - 1},
    {"/js/js2.js",          responseData_js2,   sizeof(responseData_js2) - 1},
    {"/js/js3.js",          responseData_js1,   sizeof(responseData_js1) - 1},
    {"/js/js4.js",          responseData_js2,   sizeof(responseData_js2) - 1},
    {"/404.html",           responseData_404,   sizeof(responseData_404) - 1},
};

/**
 * \brief           Open file from file system
 * \param[in]       hi: HTTP init structure
 * \param[in]       file: Pointer to file structure
 * \param[in]       path: File path to open
 * \return          1 on success or 0 otherwise
 */
uint8_t
http_fs_data_open_file(const http_init_t* hi, http_fs_file_t* file, const char* path) {
    uint8_t i, res;
                                           
    file->fptr = 0;     
    if (hi != NULL && hi->fs_open != NULL) {    /* Is user defined file system ready? */
        file->rem_open_files = &http_fs_opened_files_cnt;   /* Set pointer to opened files */
        res = hi->fs_open(file, path);          /* Try to read file from user file system */
        if (res) {
            http_fs_opened_files_cnt++;         /* Increase number of opened files */
            
            file->is_static = 0;                /* File is not static */
            return 1;                           /* File is opened! */
        }
    }
    
    /*
     * Try to open static file if available
     */
    for (i = 0; i < ESP_ARRAYSIZE(http_fs_static_files); i++) {
        if (path != NULL && !strcmp(http_fs_static_files[i].path, path)) {
            memset(file, 0x00, sizeof(*file));
            
            file->size = http_fs_static_files[i].size;
            file->data = (uint8_t *)http_fs_static_files[i].data;
            file->is_static = 1;    /* Set to 0 for testing purposes */
            return 1;
        }
    }
    return 0;
}

/**
 * \brief           Read part of file or check if we have more data to read
 * \param[in]       hi: HTTP init structure
 * \param[in]       file: File handle
 * \param[in]       buff: Pointer to buffer to save read data
 * \param[in]       btr: Number of bytes to read and write to buffer
 * \param[out]      br: Pointer to save number of bytes read to buffer
 * \return          Number of bytes read or number of remaining bytes ready to be read
 */
uint32_t
http_fs_data_read_file(const http_init_t* hi, http_fs_file_t* file, void** buff, size_t btr, size_t* br) {
    uint32_t len;
    
    len = file->size - file->fptr;              /* Calculate remaining length */
    if (buff == NULL) {                         /* If there is no buffer */
        if (file->is_static) {                  /* Check static file */
            return len;                         /* Simply return difference */
        } else if (hi != NULL && hi->fs_read != NULL) { /* Check for read function */
            return hi->fs_read(file, NULL, 0);  /* Call a function for dynamic file check */
        }
        return 0;                               /* No bytes to read */
    }
    
    len = ESP_MIN(btr, len);                    /* Get number of bytes we can read */
    if (file->is_static) {                      /* Is file static? */
        *buff = (void *)&file->data[file->fptr];/* Set a new address pointer only */
    } else if (hi != NULL && hi->fs_read != NULL) {
        len = hi->fs_read(file, *buff, len);    /* Read and return number of bytes read */
    } else {
        return 0;
    }
    file->fptr += len;                          /* Incrase current file pointer */
    if (br != NULL) {
        *br = len;
    }
    return len;
}

/**
 * \brief           Close file handle
 * \param[in]       hi: HTTP init structure
 * \param[in]       file: Pointer to file handle to close
 */
void
http_fs_data_close_file(const http_init_t* hi, http_fs_file_t* file) {
    if (!file->is_static && hi != NULL && hi->fs_close != NULL) {
        if (hi->fs_close(file)) {               /* Close file handle */
            http_fs_opened_files_cnt--;         /* Decrease number of files opened */
        }
    }
}

/**
 * @file   ngx_http_hello_world_module.c
 * @author António P. P. Almeida <appa@perusio.net>
 * @date   Wed Aug 17 12:06:52 2011
 *
 * @brief  A hello world module for Nginx.
 *
 * @section LICENSE
 *
 * Copyright (C) 2011 by Dominic Fallows, António P. P. Almeida <appa@perusio.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#define HELLO_WORLD    "hello world"
#define HELLO_WORLD_ES "hola mundo"
#define HELLO_WORLD_FR "bonjour monde"
#define HELLO_WORLD_IN "namaste duniya"

static char *ngx_http_hello_world(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_hello_world_handler(ngx_http_request_t *r);
static void *ngx_http_hello_world_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_hello_world_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

typedef struct {
    ngx_str_t hello_world;
} ngx_http_hello_world_loc_conf_t;

/**
 * This module provided directive: hello world.
 *
 */
static ngx_command_t ngx_http_hello_world_commands[] = {

    { ngx_string("hello_world"), /* directive */
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS|NGX_CONF_TAKE1, /* location context and takes
                                            no arguments*/
      ngx_http_hello_world, /* configuration setup function */
      0, /* No offset. Only one context is supported. */
      0, /* No offset when storing the module configuration on struct. */
      NULL},

    ngx_null_command /* command termination */
};

/* The hello world string. */
static u_char ngx_hello_world[] = HELLO_WORLD;
static u_char ngx_hello_world_es[] = HELLO_WORLD_ES;
static u_char ngx_hello_world_fr[] = HELLO_WORLD_FR;
static u_char ngx_hello_world_in[] = HELLO_WORLD_IN;

/* The module context. */
static ngx_http_module_t ngx_http_hello_world_module_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    ngx_http_hello_world_create_loc_conf, /* create location configuration */
    ngx_http_hello_world_merge_loc_conf   /* merge location configuration */
};

/* Module definition. */
ngx_module_t ngx_http_hello_world_module = {
    NGX_MODULE_V1,
    &ngx_http_hello_world_module_ctx, /* module context */
    ngx_http_hello_world_commands, /* module directives */
    NGX_HTTP_MODULE, /* module type */
    NULL, /* init master */
    NULL, /* init module */
    NULL, /* init process */
    NULL, /* init thread */
    NULL, /* exit thread */
    NULL, /* exit process */
    NULL, /* exit master */
    NGX_MODULE_V1_PADDING
};

/**
 * Content handler.
 *
 * @param r
 *   Pointer to the request structure. See http_request.h.
 * @return
 *   The status of the response generation.
 */
static ngx_int_t ngx_http_hello_world_handler(ngx_http_request_t *r)
{
    ngx_buf_t *b;
    ngx_chain_t out;
    ngx_http_hello_world_loc_conf_t *hwlc;

    hwlc = ngx_http_get_module_loc_conf(r, ngx_http_hello_world_module);
    if (hwlc == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Could not get module context");
	return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    /* Set the Content-Type header. */
    r->headers_out.content_type.len = sizeof("text/plain") - 1;
    r->headers_out.content_type.data = (u_char *) "text/plain";

    /* Allocate a new buffer for sending out the reply. */
    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

    /* Insertion in the buffer chain. */
    out.buf = b;
    out.next = NULL; /* just one buffer */


    b->pos = hwlc->hello_world.data; /* first position in memory of the data */
    b->last = hwlc->hello_world.data + hwlc->hello_world.len; /* last position in memory of the data */
    b->memory = 1; /* content is in read-only memory */
    b->last_buf = 1; /* there will be no more buffers in the request */

    /* Sending the headers for the reply. */
    r->headers_out.status = NGX_HTTP_OK; /* 200 status code */
    /* Get the content length of the body. */
    r->headers_out.content_length_n = hwlc->hello_world.len;
    ngx_http_send_header(r); /* Send the headers */

    /* Send the body, and return the status code of the output filter chain. */
    return ngx_http_output_filter(r, &out);
} /* ngx_http_hello_world_handler */

/**
 * Configuration setup function that installs the content handler.
 *
 * @param cf
 *   Module configuration structure pointer.
 * @param cmd
 *   Module directives structure pointer.
 * @param conf
 *   Module configuration structure pointer.
 * @return string
 *   Status of the configuration setup.
 */
static char *ngx_http_hello_world(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf; /* pointer to core location configuration */
    ngx_http_hello_world_loc_conf_t *hwlc;
    ngx_str_t *value = cf->args->elts;

    /* Install the hello world handler. */
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_hello_world_handler;

    /* Parse hello world language parameter */
    hwlc = ngx_http_conf_get_module_loc_conf(cf, ngx_http_hello_world_module);
    if (hwlc == NULL) {
        return NGX_CONF_ERROR;
    }

    if ((cf->args->nelts == 1) || (ngx_strcmp(value[1].data, "en") == 0)) {
	hwlc->hello_world.len = sizeof(ngx_hello_world);
	hwlc->hello_world.data = ngx_hello_world;
    } else if (ngx_strcmp(value[1].data, "es") == 0) {
	hwlc->hello_world.len = sizeof(ngx_hello_world_es);
	hwlc->hello_world.data = ngx_hello_world_es;
    } else if (ngx_strcmp(value[1].data, "fr") == 0) {
	hwlc->hello_world.len = sizeof(ngx_hello_world_fr);
	hwlc->hello_world.data = ngx_hello_world_fr;
    } else if (ngx_strcmp(value[1].data, "in") == 0) {
	hwlc->hello_world.len = sizeof(ngx_hello_world_in);
	hwlc->hello_world.data = ngx_hello_world_in;
    } else {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
} /* ngx_http_hello_world */

/**
 * Allocates memory for module context and sets up defaults
 *
 * @param cf
 *   Module configuration structure pointer.
 * @return conf
 *   Module context.
 */
static void *
ngx_http_hello_world_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_hello_world_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_hello_world_loc_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }
    conf->hello_world.len = sizeof(ngx_hello_world);
    conf->hello_world.data = ngx_hello_world;
    return conf;
} /* ngx_http_hello_world_create_loc_conf */

/**
 * Merge module configuration
 *
 * @param cf
 *   Module configuration structure pointer.
 * @param parent
 *   Parent module configuration structure pointer.
 * @param child
 *   Child module configuration structure pointer.
 * @return string
 *   Status of the configuration setup.
 */
static char *
ngx_http_hello_world_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_hello_world_loc_conf_t  *prev = parent;
    ngx_http_hello_world_loc_conf_t  *conf = child;

    ngx_conf_merge_str_value(conf->hello_world, prev->hello_world, "hello world");

    return NGX_CONF_OK;
}

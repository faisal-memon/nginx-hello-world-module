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

#define HELLO_WORLD_FILE_SIZE 2048
#define HELLO_WORLD_HASH_MAX_SIZE 256
#define HELLO_WORLD_MAX_LANGS 50
#define ACC_LANG_NOT_FOUND "Accept Language header not found or no valid languages found."

static char *ngx_http_hello_world(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_hello_world_handler(ngx_http_request_t *r);
static void *ngx_http_hello_world_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_hello_world_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

typedef struct {
    ngx_hash_t hash;
    ngx_hash_keys_arrays_t hash_keys;
} ngx_http_hello_world_loc_conf_t;

/**
 * This module provided directive: hello world.
 *
 */
static ngx_command_t ngx_http_hello_world_commands[] = {

    { ngx_string("hello_world"), /* directive */
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1, /* location context and takes
                                            no arguments*/
      ngx_http_hello_world, /* configuration setup function */
      0, /* No offset. Only one context is supported. */
      0, /* No offset when storing the module configuration on struct. */
      NULL},

    ngx_null_command /* command termination */
};

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

static int ngx_libc_cdecl
ngx_hello_world_sort_quality(const void *one, const void *two)
{
    ngx_buf_t *first = *(ngx_buf_t **) one;
    ngx_buf_t *second = *(ngx_buf_t **) two;

    ngx_uint_t first_quality = (ngx_uint_t) first->tag;
    ngx_uint_t second_quality = (ngx_uint_t) second->tag;

    return second_quality - first_quality;
}

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
    ngx_buf_t *b=NULL;
    ngx_buf_t *buf[HELLO_WORLD_MAX_LANGS];
    ngx_chain_t  *chain, *cl, **ll;
    ngx_http_hello_world_loc_conf_t *hwlc;
    ngx_uint_t key, response_size=0, size, quality, buf_index=0;
    u_char *hello_world_str, *curr_key;
    char *quality_str;

    hwlc = ngx_http_get_module_loc_conf(r, ngx_http_hello_world_module);
    if (hwlc == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Could not get module context");
	return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    /* Set the Content-Type header. */
    r->headers_out.content_type.len = sizeof("text/plain") - 1;
    r->headers_out.content_type.data = (u_char *) "text/plain";

    if (r->headers_in.accept_language != NULL) {
	curr_key = (u_char *) strtok((char *) r->headers_in.accept_language->value.data, ", ");
	ll = &chain;

	while (curr_key != NULL && buf_index < HELLO_WORLD_MAX_LANGS) {
	    /* Search for quality value ie ;q=0.5 */
	    quality=10;
	    quality_str = ngx_strchr(curr_key, ';');
	    if (quality_str != NULL) {
	        *quality_str++ = '\0';
                sscanf(quality_str, "q=0.%1ld", &quality);
	    }
	    
            key = ngx_hash_key_lc(curr_key, ngx_strlen(curr_key));
	    ngx_strlow(curr_key, curr_key, ngx_strlen(curr_key));
	    hello_world_str = ngx_hash_find(&hwlc->hash, key, curr_key, ngx_strlen(curr_key));
	    if (hello_world_str != NULL) {
                size = ngx_strlen(hello_world_str) + 2; /* Append \n */
	        response_size += size - 1;
                b = ngx_create_temp_buf(r->pool, size);
	        if (b == NULL) {
                    return NGX_HTTP_INTERNAL_SERVER_ERROR;
                }
	        b->last = ngx_sprintf(b->last, "%s\n", hello_world_str);
		b->tag = (void *) quality;
                b->last_buf = 0; /* there will be more buffers in the request */
		buf[buf_index++] = b;
	    }

	    curr_key = (u_char *) strtok(NULL, ", ");
	}
	ngx_qsort(buf, buf_index, sizeof(ngx_buf_t *), ngx_hello_world_sort_quality);

	for (ngx_uint_t i = 0; i < buf_index; i++) {
	    cl = ngx_alloc_chain_link(r->pool);
	    if (cl == NULL) {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }
	    cl->buf = buf[i];
	    *ll = cl;
	    ll = &cl->next;
	}
	*ll = NULL;
	if (buf_index != 0) {
	    buf[buf_index-1]->last_buf = 1;
	}
    }

    if (response_size == 0) {
	size = sizeof(ACC_LANG_NOT_FOUND);
	response_size += size - 1;

	chain = ngx_alloc_chain_link(r->pool);
	if (chain == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
	b->pos = (u_char *) ACC_LANG_NOT_FOUND;
	b->last = (u_char *) ACC_LANG_NOT_FOUND + size - 1;
        b->memory = 1; /* content is in read-only memory */
        b->last_buf = 1; /* there will be no more buffers in the request */
	chain->buf = b;
	chain->next = NULL;
    }

    /* Sending the headers for the reply. */
    r->headers_out.status = NGX_HTTP_OK; /* 200 status code */
    /* Get the content length of the body. */
    r->headers_out.content_length_n = response_size;
    ngx_http_send_header(r); /* Send the headers */

    /* Send the body, and return the status code of the output filter chain. */
    return ngx_http_output_filter(r, chain);
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
    char *file_buf, *lang_code, *hello_world_str;
    size_t size;
    ngx_file_t file;
    ngx_file_info_t fi;
    const char delim_lang[2] = " ";
    const char delim_hello[2] = "\n";
    ngx_str_t key;
    ngx_hash_init_t hash;

    /* Install the hello world handler. */
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_hello_world_handler;

    /* Parse hello world language parameter */
    hwlc = ngx_http_conf_get_module_loc_conf(cf, ngx_http_hello_world_module);
    if (hwlc == NULL) {
        return NGX_CONF_ERROR;
    }

    /* Open up the file and read it into a buffer */
    ngx_memzero(&file, sizeof(ngx_file_t));
    file.name = value[1];
    file.log = cf->log;

    file.fd = ngx_open_file(file.name.data, NGX_FILE_RDONLY, NGX_FILE_OPEN, 0);
    if (file.fd == NGX_INVALID_FILE) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,"File %s not found", file.name.data);
        return NGX_CONF_ERROR;
    }

    if (ngx_fd_info(file.fd, &fi) == NGX_FILE_ERROR) {
        ngx_conf_log_error(NGX_LOG_CRIT, cf, ngx_errno,
                           ngx_fd_info_n " \"%s\" failed", file.name.data);
        return NGX_CONF_ERROR;
    }

    size = (size_t) ngx_file_size(&fi);
    if (size > HELLO_WORLD_FILE_SIZE) {
        size = HELLO_WORLD_FILE_SIZE;
    }	

    file_buf = ngx_pcalloc(cf->pool, size+1);
    if (file_buf == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_read_file(&file, (u_char *)file_buf, size, 0);

    if (ngx_close_file(file.fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, file.log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", file.name.data);
    }

    /* Tokenize the buffer */
    lang_code = strtok(file_buf, delim_lang);
    hello_world_str = strtok(NULL, delim_hello);
    key.data = (u_char *) lang_code;
    key.len  = strlen(lang_code);
    ngx_hash_add_key(&hwlc->hash_keys, &key, hello_world_str, 0);

    while ((lang_code = strtok(NULL, delim_lang)) != NULL && (hello_world_str = strtok(NULL, delim_hello)) != NULL) {
        key.data = (u_char *) lang_code;
        key.len  = strlen(lang_code);
        if (ngx_hash_add_key(&hwlc->hash_keys, &key, hello_world_str, 0) != NGX_OK) {
            ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "Error adding key");
            return NGX_CONF_ERROR;
	}
    }

    /* Initialize the Hash Table */
    hash.hash = &hwlc->hash;
    hash.key = ngx_hash_key_lc;
    hash.max_size = HELLO_WORLD_HASH_MAX_SIZE;
    hash.bucket_size = ngx_cacheline_size;
    hash.name = "hello_world_hash";
    hash.pool = cf->pool;
    hash.temp_pool = cf->temp_pool;

    if (ngx_hash_init(&hash, hwlc->hash_keys.keys.elts, hwlc->hash_keys.keys.nelts) != NGX_OK) {
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
    
    conf->hash_keys.pool = cf->pool;
    conf->hash_keys.temp_pool = cf->temp_pool;

    ngx_hash_keys_array_init(&conf->hash_keys, NGX_HASH_SMALL);

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
    return NGX_CONF_OK;
}

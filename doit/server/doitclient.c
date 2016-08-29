/*
 * DoIt client.
 * 
 * Copyright 2000-2001 Simon Tatham. All rights reserved.
 * 
 * You may copy and use this file under the terms of the MIT
 * Licence. For details, see the file LICENCE provided in the DoIt
 * distribution archive. At the time of writing, a copy of the
 * licence is also available at
 * 
 *   http://www.opensource.org/licenses/mit-license.html
 * 
 * but this cannot be guaranteed not to have changed in the future.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>

#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "doit.h"
#include "config.h"

char *dupstr(char *s)
{
    char *ret = malloc(1+strlen(s));
    if (!ret) {
        perror("malloc");
        exit(1);
    }
    strcpy(ret, s);
    return ret;
}

/* Allocate the concatenation of N strings. Terminate arg list with NULL. */
char *dupcat(char *s1, ...)
{
    int len;
    char *p, *q, *sn;
    va_list ap;

    len = strlen(s1);
    va_start(ap, s1);
    while (1) {
	sn = va_arg(ap, char *);
	if (!sn)
	    break;
	len += strlen(sn);
    }
    va_end(ap);

    p = malloc(len + 1);
    if (!p) {
	perror("malloc");
	exit(1);
    }
    strcpy(p, s1);
    q = p + strlen(p);

    va_start(ap, s1);
    while (1) {
	sn = va_arg(ap, char *);
	if (!sn)
	    break;
	strcpy(q, sn);
	q += strlen(q);
    }
    va_end(ap);

    return p;
}

char *get_pwd(void)
{
    char *buf, *ret;
    int size = FILENAME_MAX;

    buf = malloc(size);
    while ( (ret = getcwd(buf, size)) == NULL && errno == ERANGE) {
        size *= 2;
        buf = realloc(buf, size);
    }
    return buf;
}

void *read_secret_file(char *fname, int *len) {
    FILE *fp;
    void *secret;
    int secretlen;

    fp = fopen(fname, "rb");
    if (!fp) {
        perror(fname);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    secretlen = ftell(fp);
    secret = malloc(secretlen);
    if (!secret) {
        perror("malloc");
        return NULL;
    }
    fseek(fp, 0, SEEK_SET);
    if (!fread(secret, 1, secretlen, fp)) {
        perror("read");
        return NULL;
    }
    fclose(fp);

    *len = secretlen;
    return secret;
}

int make_connection(unsigned long ipaddr, int port)
{
    int sock;
    unsigned long a;
    struct sockaddr_in addr;

#if 0
    struct hostent *h;
    a = inet_addr(host);
    if (a == (unsigned long)-1) {
        h = gethostbyname(host);
        if (!h) {
            perror(host);
            return -1;
        }
        memcpy(&a, h->h_addr, sizeof(a));
    }
#endif
    a = ntohl(ipaddr);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(0);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    addr.sin_addr.s_addr = htonl(a);
    addr.sin_port = htons(port);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return -1;
    }

    return sock;
}

int do_send(int sock, void *buffer, int len)
{
    char *buf = (char *)buffer;
    int ret, sent;

    sent = 0;
    while (len > 0 && (ret = send(sock, buf, len, 0)) > 0) {
        buf += ret;
        len -= ret;
        sent += ret;
    }
    if (ret <= 0)
        return ret;
    else
        return sent;
}

int do_receive(int sock, doit_ctx *ctx)
{
    char buf[1024];
    int ret;

    ret = recv(sock, buf, sizeof(buf), 0);
    if (ret > 0)
        doit_incoming_data(ctx, buf, ret);
    return ret;
}

int do_fetch_pascal_string(int sock, doit_ctx *ctx, char *buf)
{
    int x = doit_incoming_data(ctx, NULL, 0);
    unsigned char len;
    int ret;
    while (x == 0) {
        if ((ret = do_receive(sock, ctx)) <= 0) {
            if (ret == 0) {
                fprintf(stderr, "doit: connection unexpectedly closed\n");
            } else {
                fprintf(stderr, "doit: network error: %s\n", strerror(errno));
            }
            return -1;
        }
        x = doit_incoming_data(ctx, NULL, 0);
    }
    if (doit_read(ctx, &len, 1) != 1)
        return -1;
    x--;
    while (x < len) {
        if ((ret = do_receive(sock, ctx)) <= 0) {
            if (ret == 0) {
                fprintf(stderr, "doit: connection unexpectedly closed\n");
            } else {
                fprintf(stderr, "doit: network error: %s\n", strerror(errno));
            }
            return -1;
        }
        x = doit_incoming_data(ctx, NULL, 0);
    }
    if (doit_read(ctx, buf, len) != len)
        return -1;
    return len;
}

/*
 * Helper function to fetch a whole line from the socket.
 */
char *do_fetch(int sock, doit_ctx *ctx, int line_terminate, int maxlen,
	       int *length)
{
    char *cmdline = NULL;
    int cmdlen = 0, cmdsize = 0;
    char buf[1024];
    int len;

    /*
     * Start with any existing buffered data.
     */
    len = doit_incoming_data(ctx, NULL, 0);
    cmdline = malloc(256);
    cmdlen = 0;
    cmdsize = 256;
    while (1) {
        if (len > 0) {
            if (cmdsize < cmdlen + len + 1) {
                cmdsize = cmdlen + len + 1 + 256;
                cmdline = realloc(cmdline, cmdsize);
                if (!cmdline)
                    return NULL;
            }
            while (len > 0) {
                doit_read(ctx, cmdline+cmdlen, 1);
                if (line_terminate &&
                    cmdlen > 0 && cmdline[cmdlen] == '\n') {
                    cmdline[cmdlen] = '\0';
                    *length = cmdlen;
                    return cmdline;
                }
                cmdlen++;
                len--;
		if (maxlen != 0 && cmdlen >= maxlen) {
                    cmdline[cmdlen] = '\0';
                    *length = cmdlen;
                    return cmdline;
		}
            }
        }
        len = recv(sock, buf, sizeof(buf), 0);
        if (len <= 0) {
            *length = cmdlen;
            return line_terminate ? NULL : cmdline;
        }
        len = doit_incoming_data(ctx, buf, len);
    }
}
char *do_fetch_line(int sock, doit_ctx *ctx)
{
    int len;
    return do_fetch(sock, ctx, 1, 0, &len);
}
char *do_fetch_all(int sock, doit_ctx *ctx, int *len)
{
    return do_fetch(sock, ctx, 0, 0, len);
}
char *do_fetch_n(int sock, doit_ctx *ctx, int length, int *len)
{
    return do_fetch(sock, ctx, 0, length, len);
}

/*
 * Helper functions to encrypt and send data.
 */
int do_doit_send(int sock, doit_ctx *ctx, void *data, int len)
{
    void *odata;
    int olen;
    odata = doit_send(ctx, data, len, &olen);
    if (odata) {
        int ret = do_send(sock, odata, olen);
        free(odata);
        return ret;
    } else {
        return -1;
    }
}
int do_doit_send_str(int sock, doit_ctx *ctx, char *str)
{
    return do_doit_send(sock, ctx, str, strlen(str));
}

/*
 * Parse the command-line verb.
 */
enum {
    NOTHING,                           /* invalid command */
    WF,                                /* path-translation + ShellExecute */
    WWW,                               /* start a browser */
    WCLIP,                             /* read/write the clipboard */
    WCMD,                              /* run a process with output */
    WIN,                               /* just run a process */
    WINWAIT,			       /* run a process and wait for completion */
    WPATH,                             /* just translate a path */
};

#define need_connection ( cmd != WPATH )

int parse_cmd(char *verb)
{
    if (!strcmp(verb, "wf")) return WF;
    if (!strcmp(verb, "www")) return WWW;
    if (!strcmp(verb, "wclip")) return WCLIP;
    if (!strcmp(verb, "wcmd")) return WCMD;
    if (!strcmp(verb, "win")) return WIN;
    if (!strcmp(verb, "winwait")) return WINWAIT;
    if (!strcmp(verb, "wpath")) return WPATH;
    return NOTHING;
}

/*
 * Path name translation.
 */
struct mapping {
    char *from;
    char *to;
};
struct hostcfg {
    char *name;
    int nmappings;
    struct mapping *mappings;
};
struct hostcfg *hostcfgs = NULL;
int nhostcfgs = 0;

struct hostcfg *cfg = NULL;

void slashmap(char *p)
{
    p += strcspn(p, "/");
    while (*p) {
        *p = '\\';
        p += strcspn(p, "/");
    }
}

void new_hostcfg(char *name)
{
    nhostcfgs++;
    if (hostcfgs == NULL) {
        hostcfgs = malloc(nhostcfgs * sizeof(*hostcfgs));
    } else {
        hostcfgs = realloc(hostcfgs, nhostcfgs * sizeof(*hostcfgs));
    }
    hostcfgs[nhostcfgs-1].name = dupstr(name);
    hostcfgs[nhostcfgs-1].nmappings = 0;
    hostcfgs[nhostcfgs-1].mappings = NULL;
}

void add_pathmap(char *from, char *to)
{
    struct hostcfg *h;
    if (nhostcfgs == 0)
        return;
    h = &hostcfgs[nhostcfgs-1];
    h->nmappings++;
    if (h->mappings == NULL) {
        h->mappings = malloc(h->nmappings * sizeof(*h->mappings));
    } else {
        h->mappings = realloc(h->mappings,
                              h->nmappings * sizeof(*h->mappings));
    }
    h->mappings[h->nmappings-1].from = dupstr(from);
    h->mappings[h->nmappings-1].to = dupstr(to);
    /* Do \ -> / mapping here */
    slashmap(h->mappings[h->nmappings-1].from);
    slashmap(h->mappings[h->nmappings-1].to);
}

void select_hostcfg(char *name)
{
    int i;
    for (i = 0; i < nhostcfgs; i++) {
        if (strncmp(name, hostcfgs[i].name, strlen(hostcfgs[i].name)) == 0) {
            cfg = &hostcfgs[i];
            return;
        }
    }
}

char *path_translate(char *path)
{
    int i, pathlen;

    if (!cfg)
        return dupstr(path);

    if (*path && path[strlen(path)-1] == '/')
        path = dupstr(path);
    else
        path = dupcat(path, "/", NULL);

    slashmap(path);
    for (i = 0; i < cfg->nmappings; i++) {
        char *from = cfg->mappings[i].from;
        int fromlen = strlen(from);
        if (strncmp(path, from, fromlen) == 0) {
            char *newpath;
            newpath = malloc(strlen(cfg->mappings[i].to) +
                             strlen(path+fromlen) + 1);
            strcpy(newpath, cfg->mappings[i].to);
            strcat(newpath, path+fromlen);
            free(path);
            path = newpath;
        }
    }

    pathlen = strlen(path);
    if (pathlen > 1 && path[pathlen-1] == '\\' && path[pathlen-2] != ':')
	path[pathlen-1] = '\0';

    return path;
}

void help(void)
{
    char *help[] = {
	"usage: [doitclient] subcommand [options] [parameters]",
	"options are:",
	"  -h, --help         display this text",
	"  -V, --version      display doitclient version",
#ifdef FIXME_LICENCE
	"  -L, --licence, --license    display doitclient licence terms",
#endif
	"  -d, --set-dir      run program in your current directory",
	"  -D, --no-set-dir   run program in DoIt's default directory",
	"  -p, --path-translate    do path translation on command name",
	"  -P, --no-path-translate    do not do path translation on command name",
	"  -v, --verbose      log the transaction to stderr",
	"subcommands are:",
	"  win <command>      start Windows GUI process and don't wait",
	"  winwait <command>  start Windows GUI process and wait for completion",
	"  wcmd <command>     start Windows CLI process and return output",
	"  wf <file>          path-translate a file and open in Windows",
	"  www <URL>          open a URL in Windows's default browser",
	"  wclip              copy stdin to the Windows clipboard",
	"  wclip -r           copy the Windows clipboard to stdout",
	"  wpath              path-translate a filename and print to stdout",
    };
    int i;
    for (i = 0; i < sizeof(help)/sizeof(*help); i++)
	puts(help[i]);
}

void showversion(void)
{
    char versionbuf[80];
    char *v;

#ifdef PACKAGE_VERSION
    printf("doitclient version %s\n", PACKAGE_VERSION);
#else
    printf("doitclient unknown version\n");
#endif
}

/*
 * Do path translation including prefixing the current directory.
 */
char *do_path_translate(char *arg, int verbose)
{
    char *dir, *path, *path2;

    if (verbose) {
	fprintf(stderr, "doit: need to translate path \"%s\"\n", arg);
    }

#ifdef HAVE_REALPATH
#if !defined PATH_MAX && defined MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#endif
    path2 = malloc(PATH_MAX);
    path = realpath(arg, path2);
    if (!path) {
#endif
	if (arg[0] == '/') {
	    path = dupstr(arg);
	} else {
	    dir = get_pwd();
	    path = dupcat(dir, "/", arg, NULL);
	    free(dir);
	}
#ifdef HAVE_REALPATH
    }
#endif

    if (verbose) {
	fprintf(stderr, "doit: canonification gives \"%s\"\n", path);
    }

    path2 = path_translate(path);
    free(path);

    if (verbose) {
	fprintf(stderr, "doit: path translation gives \"%s\"\n", path2);
    }

    return path2;
}

/*
 * Send a SetDirectory modifier.
 */
void set_dir(int sock, doit_ctx *ctx, int verbose)
{
    char *dir, *path;

    dir = get_pwd();
    if (verbose) {
        fprintf(stderr, "doit: current dir is \"%s\"\n", dir);
    }
    path = path_translate(dir);
    free(dir);
    if (verbose) {
        fprintf(stderr, "doit: path translation gives \"%s\"\n", path);
    }
    do_doit_send_str(sock, ctx, "SetDirectory\n");
    do_doit_send_str(sock, ctx, path);
    do_doit_send_str(sock, ctx, "\n");
    if (verbose) {
	fprintf(stderr, "doit: >>> SetDirectory\ndoit: >>> %s\n", path);
    }
    free(path);
}

int main(int argc, char **argv)
{
    int sock;
    doit_ctx *ctx;
    void *data, *secretdata;
    int len, secretlen;
    FILE *fp;
    char fname[FILENAME_MAX];
    char pbuf[256];
    char *msg, *dir, *path;

    unsigned long hostaddr;
    char *secret = NULL;
    char *browser = NULL;
    int port = DOIT_PORT;

    int cmd = 0;
    char *arg = NULL, *arg2 = NULL;
    int clip_read = 0;
    int nogo = 0, errs = 0;
    int errcode;
    int verbose = 0;
    int doing_opts = 1;

    typedef enum { TRI_MAYBE, TRI_NO, TRI_YES } troolean;

    troolean set_directory = TRI_MAYBE;
    troolean path_translation = TRI_MAYBE;

    /*
     * Read the config file `.doitrc'.
     */
    if (!getenv("HOME")) {
	fprintf(stderr, "doit: unable to read $HOME/.doitrc when HOME "
		"is not defined\n");
	exit(1);
    }
    strcpy(fname, getenv("HOME"));
    strcat(fname, "/.doitrc");
    fp = fopen(fname, "r");
    if (fp) {
        char buf[512];
        while (fgets(buf, sizeof(buf), fp)) {
            char *cmd, *arg;
            buf[strcspn(buf, "\r\n")] = '\0';
            cmd = buf + strspn(buf, " \t");
            arg = cmd + strcspn(cmd, " \t");
            if (*arg) *arg++ = '\0';
            arg += strspn(arg, " \t");

            if (!strcmp(cmd, "secret"))
                secret = dupstr(arg);
            else if (!strcmp(cmd, "host"))
                new_hostcfg(arg);
            else if (!strcmp(cmd, "map")) {
                char *arg2;
                arg2 = arg + strcspn(arg, " \t");
                if (*arg2) *arg2++ = '\0';
                arg2 += strspn(arg2, " \t");
                add_pathmap(arg, arg2);
            }
        }
        fclose(fp);
    }

    /*
     * Parse the command line to find out what to actually do.
     */
    arg = strrchr(argv[0], '/');
    if (!arg)
	arg = argv[0];
    else
	arg++;
    cmd = parse_cmd(arg);	       /* are we called as wf, wclip etc? */
    arg = NULL;
    /*
     * Parse command line arguments.
     */
    while (--argc) {
	char *p = *++argv;
	if (*p == '-' && doing_opts) {
	    /*
	     * An option.
	     */
	    while (p && *++p) {
		char c = *p;
		switch (c) {
		  case '-':
		    /*
		     * Long option.
		     */
                    if (!p[1]) {
                        /*
                         * "--" terminates option processing.
                         */
                        doing_opts = 0;
                        break;
                    }
		    {
			char *opt, *val;
			opt = p++;     /* opt will have _one_ leading - */
			while (*p && *p != '=')
			    p++;	       /* find end of option */
			if (*p == '=') {
			    *p++ = '\0';
			    val = p;
			} else
			    val = NULL;
			if (!strcmp(opt, "-help")) {
			    help();
			    nogo = 1;
			} else if (!strcmp(opt, "-version")) {
			    showversion();
			    nogo = 1;
#ifdef FIXME_LICENCE
			} else if (!strcmp(opt, "-licence") ||
				   !strcmp(opt, "-license")) {
			    licence();
			    nogo = 1;
#endif
			} else if (!strcmp(opt, "-set-dir")) {
			    set_directory = TRI_YES;
			} else if (!strcmp(opt, "-no-set-dir")) {
			    set_directory = TRI_NO;
			} else if (!strcmp(opt, "-path-translate")) {
			    path_translation = TRI_YES;
			} else if (!strcmp(opt, "-no-path-translate")) {
			    path_translation = TRI_NO;
			}
			/*
			 * A sample option requiring an argument:
			 * 
			 * else if (!strcmp(opt, "-output")) {
			 *     if (!val)
			 *         errs = 1, error(err_optnoarg, opt);
			 *     else
			 *         ofile = val;
			 * }
			 */
			else {
			    errs = 1;
                            fprintf(stderr,
                                    "doit: no such option \"-%s\"\n", opt);
			}
		    }
		    p = NULL;
		    break;
		  case 'h':
		  case 'V':
#ifdef FIXME_LICENCE
		  case 'L':
#endif
		  case 'd': case 'D':
		  case 'p': case 'P':
                  case 'r':
                  case 'v':
		    /*
		     * Option requiring no parameter.
		     */
		    switch (c) {
		      case 'h':
			help();
			nogo = 1;
			break;
		      case 'V':
			showversion();
			nogo = 1;
			break;
#ifdef FIXME_LICENCE
		      case 'L':
			licence();
			nogo = 1;
			break;
#endif
		      case 'd':
			set_directory = TRI_YES;
			break;
		      case 'D':
			set_directory = TRI_NO;
			break;
		      case 'p':
			path_translation = TRI_YES;
			break;
		      case 'P':
			path_translation = TRI_NO;
			break;
                      case 'r':
                        clip_read = 1;
                        break;
                      case 'v':
                        verbose = 1;
                        break;
		    }
		    break;
		    /*
		     * FIXME. Put cases here for single-char
		     * options that require parameters. An example
		     * is shown, commented out.
		     */
		  /* case 'o': */
		    /*
		     * Option requiring parameter.
		     */
		    p++;
		    if (!*p && argc > 1)
			--argc, p = *++argv;
		    else if (!*p) {
			errs = 1;
                        fprintf(stderr,
                                "doit: option \"-%c\" requires an argument\n",
                                c);
		    }
		    /*
		     * Now c is the option and p is the parameter.
		     */
		    switch (c) {
			/*
			 * A sample option requiring an argument:
			 *
		         * case 'o':
			 *   ofile = p;
			 *   break;
			 */
		    }
		    p = NULL;	       /* prevent continued processing */
		    break;
		  default:
		    /*
		     * Unrecognised option.
		     */
		    {
			errs = 1;
                        fprintf(stderr, "doit: no such option \"-%c\"\n", c);
		    }
		}
	    }
	} else {
	    /*
	     * A non-option argument.
	     */
            if (cmd == 0) {
                cmd = parse_cmd(p);
                if (cmd == 0) {
                    errs = 1;
                    fprintf(stderr, "doit: unrecognised command \"%s\"\n", p);
                }
            } else {
                if (arg == NULL) {
                    arg = dupstr(p);
                } else if (arg2 == NULL) {
		    arg2 = dupstr(p);
		} else {
		    char *tmp = arg2;
		    arg2 = dupcat(arg2, " ", p, NULL);
		    free(tmp);
		}
                doing_opts = 0;
            }
	}
    }

    if (errs)
	exit(EXIT_FAILURE);
    if (nogo)
	exit(EXIT_SUCCESS);

    if (!cmd) {
        fprintf(stderr, "doit: no command verb supplied; try \"--help\"\n");
        exit(EXIT_FAILURE);
    }

    /*
     * Find out what host we should be connecting to.
     */
    {
        char *e, *prehost, *servername;
        int len, gotaddr;
        struct hostent *h;
        struct in_addr ia;
        char ipaddr_buf[16];
        unsigned long a;

        e = getenv("DOIT_HOST");
        if (e != NULL) {
            prehost = dupstr(e);
	    if (verbose)
		fprintf(stderr,
			"doit: finding host using DOIT_HOST=%s\n", e);
        } else {
            e = getenv("SSH_CLIENT");
            if (e != NULL) {
                len = strcspn(e, " ");
                prehost = dupstr(e);
                prehost[len] = '\0';
		if (verbose)
		    fprintf(stderr,
			    "doit: finding host using SSH_CLIENT=%s\n", e);
            } else {
                e = getenv("DISPLAY");
                if (e != NULL) {
                    len = strcspn(e, ":");
                    prehost = dupstr(e);
                    prehost[len] = '\0';
		    if (verbose)
			fprintf(stderr,
				"doit: finding host using DISPLAY=%s\n", e);
                } else {
		    fprintf(stderr, "doit: unable to find host; try setting"
			    " DOIT_HOST\n");
		    return 1;
		}
            }
        }

        /*
         * Now we look at $DOIT_SERVER, which might tell us the
         * server name to use for looking up path translations in
         * .doitrc. Failing that, we find the canonical name of the
         * host and use that.
         */
        servername = getenv("DOIT_SERVER");
        if (servername && verbose)
            fprintf(stderr, "doit: server name is explicitly specified: "
                    "DOIT_SERVER=%s\n", servername);

        /*
         * Now we do the DNS phase. We might need this for two
         * reasons:
         * 
         *  - if we don't have a server name, we must do a lookup
         *    to find the canonical form of `prehost', and use that
         *    as our server name. In this situation we _must_
         *    perform a DNS operation, even if we have a literal IP
         *    address for the host, because we want the canonical
         *    name. (But we can leave it as an IP address if the
         *    reverse lookup fails. No point penalising people who
         *    haven't got any DNS at all.)
         * 
         *  - if `prehost' is not a literal IP, we need to do a DNS
         *    lookup so we know where to connect to, and this is a
         *    fatal error if it fails.
         */

        a = inet_addr(prehost);
        gotaddr = 0;
        if (a != (unsigned long)-1 && a != (unsigned long)0xFFFFFFFF)
            gotaddr = 1;

        h = NULL;
        if (!gotaddr || !servername) {
            if (verbose)
                fprintf(stderr, "doit: doing DNS lookup on host name %s\n",
                        prehost);
            if (!gotaddr) {
                h = gethostbyname(prehost);
            } else {
                h = gethostbyaddr((const char *)&a, sizeof(a), AF_INET);
            }
        } else {
            if (verbose)
                fprintf(stderr, "doit: we have host IP and explicit"
                        " server name; DNS not needed\n");
        }
        if (!h) {
            /*
             * If we actually _did_ DNS, report the error and
             * potentially die.
             */
            if (!gotaddr || !servername) {
                char const *err = hstrerror(h_errno);
                if (verbose)
                    fprintf(stderr, "doit: DNS lookup failed: %s\n", err);
                /*
                 * This is only a fatal error if !gotaddr.
                 */
                if (!gotaddr) {
                    perror(prehost);
                    return -1;
                }
            }
            /*
             * If necessary, we can let our canonical server name
             * be a dotted-decimal IP address. No great loss.
             */
            if (!servername) {
                ia.s_addr = a;
                hostaddr = a;
                strcpy(ipaddr_buf, inet_ntoa(ia));
                servername = ipaddr_buf;
            }
        } else {
            if (gotaddr) {
                ia.s_addr = a;
            } else {
                memcpy(&ia, h->h_addr, sizeof(ia));
            }
            hostaddr = ia.s_addr;
            if (!servername) {
                servername = h->h_name;
            }
        }

	if (verbose)
	    fprintf(stderr, "doit: searching .doitrc for server name %s\n",
                    servername);
        select_hostcfg(servername);

        if (need_connection) {
            e = getenv("DOIT_PORT");
            if (e != NULL)
                port = atoi(e);
            if (verbose) {
                ia.s_addr = hostaddr;
                fprintf(stderr, "doit: connecting to host %s port %d\n",
                        inet_ntoa(ia), port);
            }
        }
    }

    if (need_connection) {
        if (!secret) {
            fprintf(stderr, "doit: no secret file specified\n");
            exit(EXIT_FAILURE);
        }
        secretdata = read_secret_file(secret, &secretlen);
        if (!secretdata)
            return 1;
        ctx = doit_init_ctx(secretdata, secretlen);
        if (!ctx)
            return 1;
        memset(secretdata, 0, secretlen);
        free(secretdata);

        sock = make_connection(hostaddr, port);
        if (sock < 0)
            return 1;

        if (verbose)
            fprintf(stderr, "doit: made connection\n");

        doit_perturb_nonce(ctx, "client", 6);
        {
            int pid = getpid();
            doit_perturb_nonce(ctx, &pid, sizeof(pid));
        }
        data = doit_make_nonce(ctx, &len);

        while (!doit_got_keys(ctx)) {
            int ret;
            if ((ret = do_receive(sock, ctx)) <= 0) {
                close(sock);
                if (ret == 0) {
                    fprintf(stderr, "doit: connection unexpectedly closed\n");
                } else {
                    fprintf(stderr, "doit: network error: %s\n", strerror(errno));
                }
                return 1;
            }
        }

        if (do_send(sock, data, len) != len) {
            close(sock);
            return 1;
        }
        free(data);

        if (verbose)
            fprintf(stderr, "doit: exchanged nonces\n");
    }

    switch (cmd) {
      case WF:
        if (!arg) {
            fprintf(stderr, "doit: \"wf\" requires an argument\n");
            exit(EXIT_FAILURE);
        }
	/* For wf, we do path translation on TRI_MAYBE. */
	if (path_translation != TRI_NO) {
	    path = do_path_translate(arg, verbose);
	} else {
	    path = dupstr(arg);
	}
	/* For wf, we do not do directory changing on TRI_MAYBE. */
	if (set_directory == TRI_YES) {
	    set_dir(sock, ctx, verbose);
	}
	if (arg2) {
	    do_doit_send_str(sock, ctx, "ShellExecuteArgs\n");
	    do_doit_send_str(sock, ctx, path);
	    do_doit_send_str(sock, ctx, "\n");
	    do_doit_send_str(sock, ctx, arg2);
	    do_doit_send_str(sock, ctx, "\n");
	    if (verbose)
		fprintf(stderr, "doit: >>> ShellExecuteArgs\n"
			"doit: >>> %s\ndoit: >>> %s\n", path, arg2);
	} else {
	    do_doit_send_str(sock, ctx, "ShellExecute\n");
	    do_doit_send_str(sock, ctx, path);
	    do_doit_send_str(sock, ctx, "\n");
	    if (verbose)
		fprintf(stderr, "doit: >>> ShellExecute\ndoit: >>> %s\n",
			path);
	}
        break;
      case WIN:
      case WINWAIT:
        if (!arg) {
            fprintf(stderr, "doit: \"win%s\" requires an argument\n",
		    (cmd == WINWAIT ? "wait" : ""));
            exit(EXIT_FAILURE);
        }
	/* For win, we do not do path translation on TRI_MAYBE. */
	if (path_translation == TRI_YES) {
	    path = do_path_translate(arg, verbose);
	} else {
	    path = dupstr(arg);
	}
	/* For win, we do directory changing on TRI_MAYBE. */
	if (set_directory != TRI_NO) {
	    set_dir(sock, ctx, verbose);
	}
	if (arg2)
	    path = dupcat(path, " ", arg2, NULL);
        do_doit_send_str(sock, ctx, 
			 cmd == WINWAIT ? "CreateProcessWait\n" :
			 "CreateProcessNoWait\n");
        do_doit_send_str(sock, ctx, path);
        do_doit_send_str(sock, ctx, "\n");
	if (verbose)
	    fprintf(stderr, "doit: >>> CreateProcess%sWait\ndoit: >>> %s\n",
		    cmd == WINWAIT ? "" : "No", path);
        break;
      case WWW:
        if (!arg) {
            fprintf(stderr, "doit: \"www\" requires an argument\n");
            exit(EXIT_FAILURE);
        }
        do_doit_send_str(sock, ctx, "ShellExecute\n");
        do_doit_send_str(sock, ctx, arg);
        do_doit_send_str(sock, ctx, "\n");
	if (verbose)
	    fprintf(stderr, "doit: >>> ShellExecute\ndoit: >>> %s\n", arg);
        break;
      case WCLIP:
        if (clip_read) {
	    int len, clen;
	    char *p;

            do_doit_send_str(sock, ctx, "ReadClipboard\n");
	    if (verbose)
		fprintf(stderr, "doit: >>> ReadClipboard\n");

	    msg = do_fetch_n(sock, ctx, 4, &len);
	    clen = GET_32BIT_MSB_FIRST(msg);
	    free(msg);
	    if (verbose)
		fprintf(stderr, "doit: received clipboard data length %d\n",
			clen);
            if (clen == 0)
                break;
	    msg = do_fetch_n(sock, ctx, clen, &len);
	    p = msg;
	    if (verbose)
		fprintf(stderr, "doit: received %d bytes of clipboard data\n",
			len);

	    while (len > 0) {
		char *q = memchr(p, '\r', len);
		if (!q) {
		    fwrite(p, 1, len, stdout);
		    break;
		}
		fwrite(p, 1, q-p, stdout);
		len -= q-p;
		p = q;
		if (len > 1 && p[0] == '\r' && p[1] == '\n') {
		    fputc('\n', stdout);
		    p += 2;
		    len -= 2;
		} else {
		    fputc('\r', stdout);
		    p++;
		    len--;
		}
	    }
        } else {
            char buf[512];
	    int size = 0;
            do_doit_send_str(sock, ctx, "WriteClipboard\n");
	    if (verbose)
		fprintf(stderr, "doit: >>> WriteClipboard\n");
            while (fgets(buf, sizeof(buf), stdin)) {
                int newlinepos = strcspn(buf, "\n");
                do_doit_send(sock, ctx, buf, newlinepos);
		size += newlinepos;
                if (buf[newlinepos]) {
                    do_doit_send_str(sock, ctx, "\r\n");
		    size += 2;
                }
            }
            shutdown(sock, 1);
	    fprintf(stderr, "doit: sent %d bytes of clipboard data\n", size);
        }
        break;
      case WCMD:
        if (!arg) {
            fprintf(stderr, "doit: \"wcmd\" requires an argument\n");
            exit(EXIT_FAILURE);
        }
	/* For wcmd, we do not do path translation on TRI_MAYBE. */
	if (path_translation == TRI_YES) {
	    path = do_path_translate(arg, verbose);
	} else {
	    path = dupstr(arg);
	}
	/* For wcmd, we do directory changing on TRI_MAYBE. */
	if (set_directory != TRI_NO) {
	    set_dir(sock, ctx, verbose);
	}
	if (arg2)
	    path = dupcat(path, " ", arg2, NULL);
        do_doit_send_str(sock, ctx, "CreateProcessWithOutput\ncmd /c ");
        do_doit_send_str(sock, ctx, path);
        do_doit_send_str(sock, ctx, "\n");
	if (verbose)
	    fprintf(stderr,
		    "doit: >>> CreateProcessWithOutput\ndoit: >>> cmd /c %s\n", path);
        while ( (len = do_fetch_pascal_string(sock, ctx, pbuf)) > 0) {
            fwrite(pbuf, 1, len, stdout);
            fflush(stdout);
        }
        if (len < 0) {
            /* we have already printed an appropriate error message */
            close(sock);
            return 1;
        }
        break;
      case WPATH:
        if (!arg) {
            fprintf(stderr, "doit: \"wpath\" requires an argument\n");
            exit(EXIT_FAILURE);
        }
	path = do_path_translate(arg, verbose);
	printf("%s\n", path);
	return 0;
    }
    errcode = 0;
    msg = do_fetch_line(sock, ctx);
    if (!msg) {
	fprintf(stderr, "doit: remote did not send completion message\n");
	errcode = 1;
    } else {
	if (verbose)
	    fprintf(stderr, "doit: <<< %s\n", msg);
	if (msg[0] != '+') {
	    fprintf(stderr, "doit: remote error: %s\n", msg+1);
	} else if (msg[1]) {
	    /*
	     * Extract error code.
	     */
	    if (atoi(msg+1) != 0)
		errcode = 1;
	}
    }

    close(sock);
    return errcode;
}

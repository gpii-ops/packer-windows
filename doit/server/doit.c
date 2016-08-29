/*
 * DoIt: a remote-execution daemon for Windows.
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

#include <windows.h>
#include <winsock.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "doit.h"

typedef unsigned int uint32;

extern HWND listener_hwnd;
extern HINSTANCE listener_instance;

typedef struct {
    uint32 h[5];
} SHA_Core_State;

#define SHA_BLKSIZE 64

typedef struct {
    SHA_Core_State core;
    unsigned char block[SHA_BLKSIZE];
    int blkused;
    uint32 lenhi, lenlo;
} SHA_State;

void SHA_Init(SHA_State *s);
void SHA_Bytes(SHA_State *s, void *p, int len);
void SHA_Final(SHA_State *s, unsigned char *output);

static int CALLBACK AboutProc(HWND hwnd, UINT msg,
			      WPARAM wParam, LPARAM lParam);
static int CALLBACK LicenceProc(HWND hwnd, UINT msg,
				WPARAM wParam, LPARAM lParam);
HWND aboutbox = NULL;

static char *secret;
static int secretlen;
static char *secretname;
static int secret_reload;
static char dummy_secret[1] = { '\0' };

/*
 * Export the application name.
 */
char const *listener_appname = "DoIt";

/*
 * Export the list of ports to listen on.
 */
static int const port_array[] = { DOIT_PORT };
int listener_nports = sizeof(port_array) / sizeof(*port_array);
int const *listener_ports = port_array;

/*
 * Load the secret out of a file.
 */
static void load_secret(void) {
    FILE *fp;

    if (secret && secret != dummy_secret)
	free(secret);

    fp = fopen(secretname, "rb");
    if (!fp) {
        secretlen = 0;
        secret = dummy_secret;
        return;
    }
    fseek(fp, 0, SEEK_END);
    secretlen = ftell(fp);
    secret = malloc(secretlen);
    if (!secret) {
        secretlen = 0;
        secret = dummy_secret;
    }
    fseek(fp, 0, SEEK_SET);
    fread(secret, 1, secretlen, fp);
    fclose(fp);
}

/*
 * Helper function to deal with send() partially completing.
 */
static int do_send(SOCKET sock, void *buffer, int len)
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

char *write_clip(char *data, int len)
{
    HGLOBAL clipdata;
    char *lock;

    clipdata = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, len+1);

    if (!clipdata) {
        return "-unable to allocate clipboard memory\n";
    }
    if (!(lock = GlobalLock(clipdata))) {
        GlobalFree(clipdata);
        return "-unable to lock clipboard memory\n";
    }

    memcpy(lock, data, len);
    lock[len] = '\0';                  /* trailing null */

    GlobalUnlock(clipdata);
    if (OpenClipboard(listener_hwnd)) {
        EmptyClipboard();
        SetClipboardData(CF_TEXT, clipdata);
        CloseClipboard();
        return "+\n";
    } else {
        GlobalFree(clipdata);
        return "-unable to write to clipboard\r\n";
    }
}

char *read_clip(int *is_err)
{
    HGLOBAL clipdata;
    char *s;

    if (!OpenClipboard(NULL)) {
        *is_err = 1;
        return "-unable to read clipboard\r\n";
    }
    clipdata = GetClipboardData(CF_TEXT);
    CloseClipboard();
    if (!clipdata) {
        *is_err = 1;
        return "-clipboard contains no text\r\n";
    }
    s = GlobalLock(clipdata);
    if (!s) {
        *is_err = 1;
        return "-unable to lock clipboard memory\r\n";
    }
    *is_err = 0;
    return s;
}

struct process {
    char *error;
    HANDLE fromchild;
    HANDLE hproc;
};

struct process start_process(char *cmdline, int wait, int output, char *dir)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    HANDLE fromchild, tochild;
    HANDLE childout, parentout, childin, parentin;
    int inherit = FALSE;
    struct process ret;

    ret.error = NULL;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.wShowWindow = SW_SHOWNORMAL;
    si.dwFlags = STARTF_USESHOWWINDOW;
    if (output) {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;
        if (!CreatePipe(&parentout, &childout, &sa, 0) ||
            !CreatePipe(&childin, &parentin, &sa, 0)) {
            ret.error = "-CreatePipe failed\n";
            return ret;
        }
        if (!DuplicateHandle(GetCurrentProcess(), parentin,
                             GetCurrentProcess(), &tochild,
                             0, FALSE, DUPLICATE_SAME_ACCESS)) {
            ret.error = "-DuplicateHandle failed\n";
            return ret;
        }
        CloseHandle(parentin);
        if (!DuplicateHandle(GetCurrentProcess(), parentout,
                             GetCurrentProcess(), &fromchild,
                             0, FALSE, DUPLICATE_SAME_ACCESS)) {
            ret.error = "-DuplicateHandle failed\n";
            return ret;
        }
        CloseHandle(parentout);
        si.hStdInput = childin;
        si.hStdOutput = si.hStdError = childout;
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.wShowWindow = SW_HIDE;
        inherit = TRUE;
        ret.fromchild = fromchild;
    }
    if (CreateProcess(NULL, cmdline, NULL, NULL, inherit,
                      CREATE_NEW_CONSOLE | NORMAL_PRIORITY_CLASS,
                      NULL, dir, &si, &pi) == 0) {
        ret.error = "-CreateProcess failed\n";
    } else {
        ret.hproc = pi.hProcess;
    }
    if (output) {
        CloseHandle(childin);
        CloseHandle(childout);
        CloseHandle(tochild);
        if (ret.error)
            CloseHandle(fromchild);
    }
    return ret;
}

int read_process_output(struct process proc, char *buf, int len)
{
    DWORD got;
    if (ReadFile(proc.fromchild, buf, len, &got, NULL) && got > 0) {
        return got;
    } else
        return 0;
}

int process_exit_code(struct process proc)
{
    DWORD exitcode;

    WaitForSingleObject(proc.hproc, INFINITE);
    if (!GetExitCodeProcess(proc.hproc, &exitcode))
        return -1;
    else
        return exitcode;
}

/*
 * Helper function to fetch a whole line from the socket.
 */
char *do_fetch(SOCKET sock, doit_ctx *ctx, int line_terminate, int *length)
{
    char *cmdline = NULL;
    int cmdlen = 0, cmdsize = 0;
    char buf[1024];
    int len;

    /*
     * Start with any existing buffered data.
     */
    len = doit_incoming_data(ctx, NULL, 0);
    if (len < 0)
        return NULL;
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
            }
        }
        len = recv(sock, buf, sizeof(buf), 0);
        if (len <= 0) {
            *length = cmdlen;
            return line_terminate ? NULL : cmdline;
        }
        len = doit_incoming_data(ctx, buf, len);
        if (len < 0)
            return NULL;
    }
}
char *do_fetch_line(SOCKET sock, doit_ctx *ctx)
{
    int len;
    return do_fetch(sock, ctx, 1, &len);
}
char *do_fetch_all(SOCKET sock, doit_ctx *ctx, int *len)
{
    return do_fetch(sock, ctx, 0, len);
}

/*
 * Helper functions to encrypt and send data.
 */
int do_doit_send(SOCKET sock, doit_ctx *ctx, void *data, int len)
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
int do_doit_send_str(SOCKET sock, doit_ctx *ctx, char *str)
{
    return do_doit_send(sock, ctx, str, strlen(str));
}

/*
 * Export the function that handles a connection.
 */
int listener_newthread(SOCKET sock, int port, SOCKADDR_IN remoteaddr) {
    int len;
    char *nonce = NULL;
    doit_ctx *ctx = NULL;
    char *cmdline = NULL, *currdir = NULL;
    DWORD threadid;

    if (secret_reload)
	load_secret();

    ctx = doit_init_ctx(secret, secretlen);
    if (!ctx)
        goto done;

    doit_perturb_nonce(ctx, "server", 6);
    doit_perturb_nonce(ctx, &remoteaddr, sizeof(remoteaddr));
    threadid = GetCurrentThreadId();
    doit_perturb_nonce(ctx, &threadid, sizeof(threadid));
    nonce = doit_make_nonce(ctx, &len);
    if (do_send(sock, nonce, len) != len)
        goto done;
    free(nonce);
    nonce = NULL;

    while (1) {

	cmdline = do_fetch_line(sock, ctx);
	if (!cmdline)
	    goto done;

	if (!strcmp(cmdline, "SetDirectory")) {
	    /*
	     * Read a second line and store it for use as the
	     * default directory of a subsequent CreateProcess or
	     * ShellExecute.
	     */
	    free(cmdline); cmdline = NULL;
	    cmdline = do_fetch_line(sock, ctx);
	    if (!cmdline)
		goto done;
	    currdir = cmdline;
	    continue;
	}

	if (!strcmp(cmdline, "ShellExecute") ||
	    !strcmp(cmdline, "ShellExecuteArgs")) {
	    int ret;
	    int with_args;
	    char *args;
	    /*
	     * Read a second line and feed it to ShellExecute(). Give
	     * back either "+" (meaning OK) or "-" followed by an error
	     * message (meaning not OK).
	     * 
	     * ShellExecuteArgs is an alternative form in which we
	     * also provide arguments to the process.
	     */
	    if (!strcmp(cmdline, "ShellExecuteArgs"))
		with_args = TRUE;
	    else
		with_args = FALSE;
	    free(cmdline); cmdline = NULL;
	    cmdline = do_fetch_line(sock, ctx);
	    if (with_args)
		args = do_fetch_line(sock, ctx);
	    else
		args = NULL;
	    ret = (int)ShellExecute(listener_hwnd, "open", cmdline, args,
				    currdir, SW_SHOWNORMAL);
	    if (args)
		free(args);
	    if (ret <= 32) {
		char *msg = "-ShellExecute failed: unknown error\n";
		if (ret == 0) msg = "-ShellExecute failed: Out of memory or resources\n";
		if (ret == ERROR_FILE_NOT_FOUND) msg = "-ShellExecute failed: File not found\n";
		if (ret == ERROR_PATH_NOT_FOUND) msg = "-ShellExecute failed: Path not found\n";
		if (ret == ERROR_BAD_FORMAT) msg = "-ShellExecute failed: Invalid .exe file\n";
		if (ret == SE_ERR_ACCESSDENIED) msg = "-ShellExecute failed: Access denied\n";
		if (ret == SE_ERR_ASSOCINCOMPLETE) msg = "-ShellExecute failed: File name association incomplete or invalid\n";
		if (ret == SE_ERR_DDEBUSY) msg = "-ShellExecute failed: DDE busy\n";
		if (ret == SE_ERR_DDEFAIL) msg = "-ShellExecute failed: DDE transaction failed\n";
		if (ret == SE_ERR_DDETIMEOUT) msg = "-ShellExecute failed: DDE transaction timed out\n";
		if (ret == SE_ERR_DLLNOTFOUND) msg = "-ShellExecute failed: DLL not found\n";
		if (ret == SE_ERR_FNF) msg = "-ShellExecute failed: File not found\n";
		if (ret == SE_ERR_NOASSOC) msg = "-ShellExecute failed: No application associated with this file type\n";
		if (ret == SE_ERR_OOM) msg = "-ShellExecute failed: Out of memory\n";
		if (ret == SE_ERR_PNF) msg = "-ShellExecute failed: Path not found\n";
		if (ret == SE_ERR_SHARE) msg = "-ShellExecute failed: Sharing violation\n";
		do_doit_send_str(sock, ctx, msg);
	    } else {
		do_doit_send_str(sock, ctx, "+\n");
	    }
	    free(cmdline); cmdline = NULL;
	    goto done;
	}

	if (!strcmp(cmdline, "WriteClipboard")) {
	    /*
	     * Read data until the connection is closed, and write it
	     * to the Windows clipboard.
	     */
	    int len;
	    char *msg;
	    free(cmdline); cmdline = NULL;
	    cmdline = do_fetch_all(sock, ctx, &len);
	    if (cmdline) {
		msg = write_clip(cmdline, len);
		free(cmdline); cmdline = NULL;
	    } else
		msg = "-error reading input\n";
	    do_doit_send_str(sock, ctx, msg);
	    goto done;
	}

	if (!strcmp(cmdline, "ReadClipboard")) {
	    /*
	     * Read the Windows Clipboard. Give back a 4-byte
	     * length followed by the text, and then send either
	     * "+\n" or "-error message\n".
	     */
	    int is_err;
	    char *data = read_clip(&is_err);
	    char length[4];

	    if (is_err) {
		/* data is an error message */
		PUT_32BIT_MSB_FIRST(length, 0);
		do_doit_send(sock, ctx, length, 4);
		do_doit_send_str(sock, ctx, data);
	    } else {
		int len = strlen(data);
		PUT_32BIT_MSB_FIRST(length, len);
		do_doit_send(sock, ctx, length, 4);
		do_doit_send(sock, ctx, data, len);
		do_doit_send_str(sock, ctx, "+\n");
		GlobalUnlock(data);
	    }
	    goto done;
	}

	if (!strcmp(cmdline, "CreateProcessNoWait") ||
	    !strcmp(cmdline, "CreateProcessWait") ||
	    !strcmp(cmdline, "CreateProcessWithOutput")) {
	    /*
	     * Read a second line and feed it to CreateProcess.
	     * Optionally, wait for it to finish, or even send output
	     * back.
	     *
	     * If output is sent back, it is sent as a sequence of
	     * Pascal-style length-prefixed strings (a single byte
	     * followed by that many characters), and finally
	     * terminated by a \0 length byte. After _that_ comes the
	     * error indication, which may be "+number\n" for a
	     * termination with exit code, or "-errmsg\n" if a system
	     * call fails.
	     */
	    int wait, output;
	    struct process proc;

	    if (!strcmp(cmdline, "CreateProcessNoWait")) wait = output = 0;
	    if (!strcmp(cmdline, "CreateProcessWait")) wait = 1, output = 0;
	    if (!strcmp(cmdline, "CreateProcessWithOutput")) wait = output = 1;
	    free(cmdline); cmdline = NULL;
	    cmdline = do_fetch_line(sock, ctx);

	    proc = start_process(cmdline, wait, output, currdir);
	    if (proc.error) {
		if (output)
		    do_doit_send(sock, ctx, "\0", 1);
		do_doit_send_str(sock, ctx, proc.error);
		goto done;
	    }
	    if (wait) {
		int err;
		if (output) {
		    char buf[256];
		    int len;
		    while ((len = read_process_output(proc, buf+1,
						      sizeof(buf)-1)) > 0) {
			buf[0] = len;
			do_doit_send(sock, ctx, buf, len+1);
		    }
		    do_doit_send(sock, ctx, "\0", 1);
		}
		err = process_exit_code(proc);
		if (err < 0) {
		    do_doit_send_str(sock, ctx, "-GetExitCodeProcess failed\n");
		} else {
		    char buf[32];
		    sprintf(buf, "+%d\n", err);
		    do_doit_send_str(sock, ctx, buf);
		}
	    } else {
		do_doit_send_str(sock, ctx, "+\n");
	    }
	}

	/*
	 * If we've reached here without `continue' or leaving the
	 * loop, we must have an unrecognised command.
	 */
	do_doit_send_str(sock, ctx, "-unrecognised command \"");
	do_doit_send_str(sock, ctx, cmdline);
	do_doit_send_str(sock, ctx, "\"\n");
    }

    done:
    if (nonce)
        free(nonce);
    if (cmdline)
        free(cmdline);
    if (currdir)
        free(currdir);
    if (ctx)
	doit_free_ctx(ctx);
    closesocket(sock);
    return 0;
}

/*
 * Export the function that gets the command line.
 */
void listener_cmdline(char *cmdline) {
    secret_reload = 0;
    while (1) {
        while (*cmdline && isspace((unsigned char)*cmdline)) cmdline++;

        if (!strncmp(cmdline, "-r", 2) &&
            (!cmdline[2] || isspace((unsigned char)cmdline[2]))) {
            secret_reload = 1;
            cmdline += 2;
            while (*cmdline && isspace((unsigned char)*cmdline)) cmdline++;
        } else if (!strncmp(cmdline, "-p", 2)) {
            int port = 0, *ports;
            cmdline += 2;
            while (*cmdline && isspace((unsigned char)*cmdline)) cmdline++;
            while (*cmdline && !isspace((unsigned char)*cmdline)) {
                if (isdigit((unsigned char)*cmdline))
                    port = port * 10 + (*cmdline - '0');
                cmdline++;
            }
            ports = malloc(sizeof(int));
            ports[0] = port;
            listener_ports = ports;
        } else
            break;
    }
    secretname = cmdline;
    if (!secret_reload)
	load_secret();
}

/* ======================================================================
 * System tray functions.
 */

#define WM_XUSER     (WM_USER + 0x2000)
#define WM_SYSTRAY   (WM_XUSER + 6)
#define WM_SYSTRAY2  (WM_XUSER + 7)
#define IDM_CLOSE    0x0010
#define IDM_ABOUT    0x0020

static HMENU systray_menu;

extern int listener_init(void) {
    BOOL res;
    NOTIFYICONDATA tnid;
    HICON hicon;

#ifdef NIM_SETVERSION
    tnid.uVersion = 0;
    res = Shell_NotifyIcon(NIM_SETVERSION, &tnid);
#endif

    tnid.cbSize = sizeof(NOTIFYICONDATA); 
    tnid.hWnd = listener_hwnd; 
    tnid.uID = 1;                      /* unique within this systray use */
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; 
    tnid.uCallbackMessage = WM_SYSTRAY;
    tnid.hIcon = hicon = LoadIcon (listener_instance, MAKEINTRESOURCE(201));
    strcpy(tnid.szTip, "DoIt (remote-execution daemon)");

    res = Shell_NotifyIcon(NIM_ADD, &tnid); 

    if (hicon) 
        DestroyIcon(hicon); 

    systray_menu = CreatePopupMenu();
    AppendMenu (systray_menu, MF_ENABLED, IDM_ABOUT, "About");
    AppendMenu(systray_menu, MF_SEPARATOR, 0, 0);
    AppendMenu (systray_menu, MF_ENABLED, IDM_CLOSE, "Close DoIt");

    return res; 
}

extern void listener_shutdown(void) {
    BOOL res; 
    NOTIFYICONDATA tnid; 
 
    tnid.cbSize = sizeof(NOTIFYICONDATA); 
    tnid.hWnd = listener_hwnd; 
    tnid.uID = 1;

    res = Shell_NotifyIcon(NIM_DELETE, &tnid); 

    DestroyMenu(systray_menu);
}

extern int listener_wndproc(HWND hwnd, UINT message,
                            WPARAM wParam, LPARAM lParam) {
    int ret;
    POINT cursorpos;                   /* cursor position */
    static int menuinprogress;

    if (message == WM_SYSTRAY && lParam == WM_RBUTTONUP) {
        GetCursorPos(&cursorpos);
        PostMessage(hwnd, WM_SYSTRAY2, cursorpos.x, cursorpos.y);
    }

    if (message == WM_SYSTRAY2) {
        if (!menuinprogress) {
            menuinprogress = 1;
            SetForegroundWindow(hwnd);
            ret = TrackPopupMenu(systray_menu,
                                 TPM_RIGHTALIGN | TPM_BOTTOMALIGN |
                                 TPM_RIGHTBUTTON,
                                 wParam, lParam, 0, listener_hwnd, NULL);
            menuinprogress = 0;
        }
    }

    if (message == WM_COMMAND && (wParam & ~0xF) == IDM_CLOSE) {
        SendMessage(hwnd, WM_CLOSE, 0, 0);
    }
    if (message == WM_COMMAND && (wParam & ~0xF) == IDM_ABOUT) {
	if (!aboutbox) {
	    aboutbox = CreateDialog(listener_instance,
				    MAKEINTRESOURCE(300),
				    NULL, AboutProc);
	    ShowWindow(aboutbox, SW_SHOWNORMAL);
	    /*
	     * Sometimes the window comes up minimised / hidden
	     * for no obvious reason. Prevent this.
	     */
	    SetForegroundWindow(aboutbox);
	    SetWindowPos(aboutbox, HWND_TOP, 0, 0, 0, 0,
			 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	}
    }
    return 1;                          /* not handled */
}

/* ======================================================================
 * The About box: show version number.
 */

#define STR2(s) #s
#define STR(s) STR2(s)

void showversion(int line, char *buffer)
{
#ifdef VERSION
    sprintf(buffer, "%.16s version %.80s", listener_appname, STR(VERSION));
#else
    sprintf(buffer, "%.16s unknown version", listener_appname);
#endif
}

static int CALLBACK AboutProc(HWND hwnd, UINT msg,
			      WPARAM wParam, LPARAM lParam)
{
    char vbuf[160];
    switch (msg) {
      case WM_INITDIALOG:
	showversion(0, vbuf);
	SetDlgItemText(hwnd, 100, vbuf);
	showversion(1, vbuf);
	SetDlgItemText(hwnd, 101, vbuf);
	showversion(2, vbuf);
	SetDlgItemText(hwnd, 102, vbuf);
	return 1;
      case WM_COMMAND:
	switch (LOWORD(wParam)) {
	  case IDOK:
	    aboutbox = NULL;
	    DestroyWindow(hwnd);
	    return 0;
	  case 112:
	    EnableWindow(hwnd, 0);
	    DialogBox(listener_instance, MAKEINTRESOURCE(301),
		      NULL, LicenceProc);
	    EnableWindow(hwnd, 1);
	    SetActiveWindow(hwnd);
	    return 0;
	}
	return 0;
      case WM_CLOSE:
	aboutbox = NULL;
	DestroyWindow(hwnd);
	return 0;
    }
    return 0;
}

/*
 * Dialog-box function for the Licence box.
 */
static int CALLBACK LicenceProc(HWND hwnd, UINT msg,
				WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
      case WM_INITDIALOG:
	return 1;
      case WM_COMMAND:
	switch (LOWORD(wParam)) {
	  case IDOK:
	    EndDialog(hwnd, 1);
	    return 0;
	}
	return 0;
      case WM_CLOSE:
	EndDialog(hwnd, 1);
	return 0;
    }
    return 0;
}

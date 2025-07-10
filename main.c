#include <stdio.h>
#include <string.h>
#include <time.h>

#define ANSI_RESET_ALL          "\x1b[0m"

#define ANSI_COLOR_BLACK        "\x1b[30m"
#define ANSI_COLOR_RED          "\x1b[31m"
#define ANSI_COLOR_GREEN        "\x1b[32m"
#define ANSI_COLOR_YELLOW       "\x1b[33m"
#define ANSI_COLOR_BLUE         "\x1b[34m"
#define ANSI_COLOR_MAGENTA      "\x1b[35m"
#define ANSI_COLOR_CYAN         "\x1b[36m"
#define ANSI_COLOR_WHITE        "\x1b[37m"

#define ANSI_BACKGROUND_BLACK   "\x1b[40m"
#define ANSI_BACKGROUND_RED     "\x1b[41m"
#define ANSI_BACKGROUND_GREEN   "\x1b[42m"
#define ANSI_BACKGROUND_YELLOW  "\x1b[43m"
#define ANSI_BACKGROUND_BLUE    "\x1b[44m"
#define ANSI_BACKGROUND_MAGENTA "\x1b[45m"
#define ANSI_BACKGROUND_CYAN    "\x1b[46m"
#define ANSI_BACKGROUND_WHITE   "\x1b[47m"

#define ANSI_STYLE_BOLD         "\x1b[1m"
#define ANSI_STYLE_ITALIC       "\x1b[3m"
#define ANSI_STYLE_UNDERLINE    "\x1b[4m"

#define NONE_PLACEHOLDER		"<none>"

#ifdef _WIN32
#include <windows.h>
void set_console() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;

    if (hOut == INVALID_HANDLE_VALUE) return;
    if (!GetConsoleMode(hOut, &dwMode)) return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#else
void set_console() {}
#endif

typedef enum Command {
	CMD_HELP,
	CMD_VERSION,
	CMD_CONFIG,
	CMD_NEW,
	CMD_COMMIT,
	CMD_SETFORMAT
} Command;

typedef struct {
    char default_branch[255];
    char initials[16];
    char repository_name[100];
	char branch_format[255];
	char commit_format[72];
	char ticket[8];
	time_t timer;
} Config;

char path[512];
char name[255];
Config cfg = {
	.default_branch = "",
	.initials = "",
	.repository_name = "",
	.branch_format = "b1.0-%i-%n",
	.commit_format = "",
	.ticket = -1,
	.timer = -1
};

int help() {
	printf(
		"usage: %s [-v | --version] [-h | --help] <command> [<args>]\n"
		"\n"
		"commands:\n"
		"   -h, --help                 Show this help message\n"
		"   -v, --version              Show version information\n"
		"   config                     Show or set the configuration for the current user.\n"
		"   new                        Creates a new branch using the ticket number, by checking out\n"
		"                              the current branch and creating a new one.\n"
		"   commit                     Commits any changes made to the repository, using the message\n"
		"                              content and recorded time.\n"
		"   setformat                  Sets the format for the branch name or commit message.\n",
		name
	);
	return 0;
}

int help_new() {
	printf(
		"usage: %s new <ticket number>\n"
		"\n"
		"description: Creates a new branch using the ticket number, by checking out the\n"
		"current branch and creating a new one.\n"
		"\n"
		"options:\n"
		"   -h, --help                 Show this help message\n",
		name
	);
	return 0;
}

int help_config() {
	printf(
		"usage: %s config\n"
		"\n"
		"description: Show or set the configuration for the current user.\n"
		"\n"
		"options:\n"
		"   -h, --help                 Show this help message\n"
		"\n"
		"configuration options:\n"
		"   Default Branch             The default branch to checkout before creating a new branch.\n"
		"   Initials                   The initials of your name to use in branch names and commit messages.\n"
		"   Repository Name            The name of the repository, used in branch names.\n",
		name
	);
	return 0;
}

int version() {
	printf("TicketBranch version 1.0.0\n");
	printf("Author: GoldenD60 / David Skillman\n");
	return 0;
}

void save_config() {
    FILE *f = fopen(path, "wb");
    fwrite(&cfg, sizeof(cfg), 1, f);
    fclose(f);
}

int load_config() {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fread(&cfg, sizeof(cfg), 1, f);
    fclose(f);
    return 1;
}

void format(
		const char *fmt,
		char *out,
		const char *n,		// %n
		const char *i, 		// %i   
		const char *r,		// %r
		const char *t,		// %t
		const char *m,		// %m
		const char *b) {	// %b
    const char *p = fmt;
    char *o = out;

    while (*p) {
        if (*p == '%' && *(p+1)) {
            p++;
            switch (*p) {
                case 'n':
                    while (*n) *o++ = *n++;
                    break;
                case 'i':
                    while (*i) *o++ = *i++;
                    break;
                case 'r':
                    while (*r) *o++ = *r++;
                    break;
                case 't':
                    while (*t) *o++ = *t++;
                    break;
                case 'm':
                    while (*m) *o++ = *m++;
                    break;
                case 'b':
                    while (*b) *o++ = *b++;
                    break;
                case '%':
                    *o++ = '%';
                    break;
                default:
                    *o++ = '%';
                    *o++ = *p;
            }
            p++;
        } else {
            *o++ = *p++;
        }
    }
    *o = '\0';
}

void format_cfg(const char *fmt, char *out, char *message, char *branch) {
	format(
		fmt,
		out,
		cfg.ticket,
		cfg.initials,
		cfg.repository_name,
		"Time unknown",
		message,
		branch
	);
}

int config() {
	printf("Leave empty to not change.\n");

	printf("The default branch is: %s\n", cfg.default_branch[0] ? cfg.default_branch : NONE_PLACEHOLDER);

	char default_branch[255];

	printf("Default branch: ");
	scanf("%[^\n]", default_branch);
	getchar();

	if (default_branch[0] != '\0')
		memcpy(cfg.default_branch, default_branch, sizeof(cfg.default_branch));

	printf("\nYour initials are: %s\n", cfg.initials[0] ? cfg.initials : NONE_PLACEHOLDER);

	char initials[16];

	printf("Initials: ");
	scanf("%[^\n]", initials);
	getchar();

	if (initials[0] != '\0')
		memcpy(cfg.initials, initials, sizeof(cfg.initials));

	printf("\nThe repository name is: %s\n", cfg.repository_name[0] ? cfg.repository_name : NONE_PLACEHOLDER);

	char repository_name[100];
	
	printf("Repository Name: ");
	scanf("%[^\n]", repository_name);
	getchar();

	if (repository_name[0] != '\0')
		memcpy(cfg.repository_name, repository_name, sizeof(cfg.repository_name));

	save_config();
	return 0;
}

int new(char ticket[8])
{
	char new_branch[255];
	
	memcpy(cfg.ticket, ticket, sizeof(cfg.ticket));
	format_cfg(cfg.branch_format, new_branch, NONE_PLACEHOLDER, NONE_PLACEHOLDER);
	printf("git fetch\n");
	printf("git checkout %s\n", cfg.default_branch);
	printf("git checkout -b %s\n", new_branch);

	save_config();
	return 0;
}

int main(int argc, char **argv)
{
	set_console();
#ifdef _WIN32
    const char *home = getenv("USERPROFILE");
#else
    const char *home = getenv("HOME");
#endif
    if (!home) home = "."; // fallback
    snprintf(path, sizeof(path), "%s/.tb_config", home);

	if (!load_config()) {
        save_config();
    }

	memcpy(name, argv[0], sizeof(name));

	if (argc > 1) {
		if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			return help();
		}
		else if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version"))
		{
			return version();
		}
		else if (!strcmp(argv[1], "config"))
		{
			if (!strcmp(argv[2], "-h") || !strcmp(argv[2], "--help"))
			{
				printf(ANSI_RESET_ALL);
				return help_config();
			}
			return config();
		}
		else if (!strcmp(argv[1], "new"))
		{
			printf(ANSI_COLOR_RED);
			printf(ANSI_STYLE_BOLD);
			if (argc < 3) {
				printf("Ticket number required\n");
				printf(ANSI_RESET_ALL);
				return help_new();
			}
			if (!strcmp(argv[2], "-h") || !strcmp(argv[2], "--help"))
			{
				printf(ANSI_RESET_ALL);
				return help_new();
			}
			if (cfg.initials[0] == '\0') {
				printf("You need to set your initials in the configuration\n");
				printf(ANSI_RESET_ALL);
				return 1;
			}
			if (cfg.default_branch[0] == '\0') {
				printf("You need to set the default branch in the configuration\n");
				printf(ANSI_RESET_ALL);
				return 1;
			}
			if (cfg.branch_format[0] == '\0') {
				printf("You need to set the branch format in the format configuration\n");
				printf(ANSI_RESET_ALL);
				return 1;
			}

			printf(ANSI_RESET_ALL);
			return new(argv[2]);
		}
		else if (!strcmp(argv[1], "commit"))
		{
			return 0;
		}
		else if (!strcmp(argv[1], "setformat"))
		{
			return 0;
		}
	}
	printf(ANSI_COLOR_RED);
	printf(ANSI_STYLE_BOLD);
	printf("Invalid command\n");
	printf(ANSI_RESET_ALL);
	return help();
}
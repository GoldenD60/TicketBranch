#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <string.h>
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
	
#define ERR_NO_INITIALS			1
#define ERR_NO_DEFAULT_BRANCH	2
#define ERR_NO_BRANCH_FORMAT	4
#define ERR_NO_TICKET			8
#define ERR_MISSING_ARG			16
#define ERR_CANCELLED			32

typedef struct {
	int version;
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
Config cfg = {0};

int help() {
	printf(
		ANSI_RESET_ALL ANSI_STYLE_BOLD"usage: %s [-v | --version] [-h | --help] <command> [<args>]\n"ANSI_RESET_ALL
		"\n"
		ANSI_STYLE_BOLD"commands:\n"ANSI_RESET_ALL
		"    -h, --help                         Show this help message\n"
		"    -v, --version                      Show version information\n"
		"    config [-r | --reset]              Show or set the configuration for the current user.\n"
		"    new <ticket>                       Creates a new branch using the ticket number, by checking out\n"
		"                                       the current branch and creating a new one.\n"
		"    commit <message>                   Commits any changes made to the repository, using the message\n"
		"                                       content and recorded time.\n"
		"    format [-c | --commit <format>]    Sets the format for the branch name or commit message.\n"
		"           [-b | --branch <format>]\n",
		name
	);
	return 0;
}

int help_config() {
	printf(
		ANSI_RESET_ALL ANSI_STYLE_BOLD"usage: %s config\n"ANSI_RESET_ALL
		"\n"
		ANSI_STYLE_BOLD"description: "ANSI_RESET_ALL"Show or set the configuration for the current user.\n"
		"\n"
		ANSI_STYLE_BOLD"options:\n"ANSI_RESET_ALL
		"    -h, --help                 Show this help message\n"
		"    -r, --reset                Reset all configuration\n"
		"\n"
		ANSI_STYLE_BOLD"configuration options:\n"ANSI_RESET_ALL
		"    Default Branch             The default branch to checkout before creating a new branch.\n"
		"    Initials                   The initials of your name to use in branch names and commit messages.\n"
		"    Repository Name            The name of the repository, used in branch names.\n",
		name
	);
	return 0;
}

int help_new() {
	printf(
		ANSI_RESET_ALL ANSI_STYLE_BOLD"usage: %s new <ticket>\n"ANSI_RESET_ALL
		"\n"
		ANSI_STYLE_BOLD"description: "ANSI_RESET_ALL"Creates a new branch using the ticket number, by checking out the\n"
		"current branch and creating a new one.\n"
		"\n"
		ANSI_STYLE_BOLD"options:\n"ANSI_RESET_ALL
		"    -h, --help                 Show this help message\n"
		"\n"
		ANSI_STYLE_BOLD"arguments:\n"ANSI_RESET_ALL
		"    <ticket>: The ticket number.\n",
		name
	);
	return 0;
}

int help_commit() {
	printf(
		ANSI_RESET_ALL ANSI_STYLE_BOLD"usage: %s commit <message>\n"ANSI_RESET_ALL
		"\n"
		ANSI_STYLE_BOLD"description: "ANSI_RESET_ALL"Commits any changes made to the repository, using the message\n"
		"             content and recorded time.\n"
		"\n"
		ANSI_STYLE_BOLD"options:\n"ANSI_RESET_ALL
		"    -h, --help                 Show this help message\n",
		name
	);
	return 0;
}

int version() {
	printf("%sTicketBranch v1.0.0\n"
	       "%sAuthor: GoldenD60 / David Skillman\n"
		   "%shttps://github.com/GoldenD60/TicketBranch%s",
			ANSI_STYLE_BOLD,
			ANSI_RESET_ALL ANSI_STYLE_ITALIC,
			ANSI_RESET_ALL ANSI_STYLE_UNDERLINE,
			ANSI_RESET_ALL);
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

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size != sizeof(Config)) {
		fclose(f);
		return 0;
	}
	if (fread(&cfg, sizeof(cfg), 1, f) != 1) {
    fclose(f);
		printf(ANSI_COLOR_RED ANSI_STYLE_BOLD"Failed to read config file (partial read).\n"ANSI_RESET_ALL);
		return 0;
	}

	if (cfg.version != 1) {
		printf(ANSI_COLOR_RED ANSI_STYLE_BOLD"Config version mismatch or corruption.\n"ANSI_RESET_ALL);
		return 0;
	}

	fclose(f);

	// Check version
	if (cfg.version != 1) return 0;
	return 1;
}

int format(char *out, size_t out_size,
		   const char *fmt,
		   const char *ticket,
		   const char *initials,
		   const char *repo,
		   const char *time,
		   const char *message,
		   const char *branch) {
    size_t i = 0, j = 0;

    while (fmt[i] && j < out_size - 1) {
        if (fmt[i] == '%' && fmt[i + 1]) {
            i++;
            const char *rep = NULL;
            switch (fmt[i]) {
                case 'n': rep = ticket; break;
                case 'i': rep = initials; break;
                case 'r': rep = repo; break;
                case 't': rep = time; break;
                case 'm': rep = message; break;
                case 'b': rep = branch; break;
                case '%': out[j++] = '%'; rep = NULL; break;
                default:  out[j++] = '%'; out[j++] = fmt[i]; rep = NULL; break;
            }
            if (rep) while (*rep && j < out_size - 1) out[j++] = *rep++;
        } else {
            out[j++] = fmt[i];
        }
        i++;
    }

    out[j] = '\0';
    return 0;
}

char *join_args(int argc, char **argv, int start) {
    if (argc <= start) return strdup("");

    int len = 0;
    for (int i = start; i < argc; i++) len += strlen(argv[i]) + 1;

    char *s = malloc(len), *p = s;
    for (int i = start; i < argc; i++)
        p += sprintf(p, "%s%s", argv[i], i < argc - 1 ? " " : "");
    
    return s;
}

int format_cfg(char *out, size_t out_size, const char *fmt, char *message, char *branch) {
	char smins[16];
	itoa((int)((time(NULL) - cfg.timer) / 60), smins, 10);
	return format(
		out,
		out_size,
		fmt,
		cfg.ticket,
		cfg.initials,
		cfg.repository_name,
		smins,
		message,
		branch
	);
}

int check_cfg() {
	int err = 0;
	if (!strcmp(cfg.initials, "")) {
		printf("You need to set your initials in the configuration\n");
		err |= ERR_NO_INITIALS;
	}
	if (!strcmp(cfg.default_branch, "")) {
		printf("You need to set the default branch in the configuration\n");
		err |= ERR_NO_DEFAULT_BRANCH;
	}
	if (!strcmp(cfg.branch_format, "")) {
		printf("You need to set the branch format in the format configuration.\nUse '%s format [-b | --branch] <format>'\n", name);
		err |= ERR_NO_BRANCH_FORMAT;
	}
	if (!strcmp(cfg.commit_format, "")) {
		printf("You need to set the commit format in the format configuration.\nUse '%s format [-c | --commit] <format>'\n", name);
		err |= ERR_NO_BRANCH_FORMAT;
	}
	return err;
}

int ask() {
	printf("Is this OK? (Y/n) ");

    char answer = '\0';
    while (answer != 'y' && answer != 'n') {
        answer = getchar();
        while (getchar() != '\n');
        answer = tolower(answer);
    }
    return answer == 'y';
}

int config() {
	printf("Leave empty to not change.\n");

	printf("The default branch is: %s\n", cfg.default_branch[0] ? cfg.default_branch : NONE_PLACEHOLDER);

	char default_branch[255];

	printf("Default branch: ");
	fgets(default_branch, sizeof(default_branch), stdin);
	default_branch[strcspn(default_branch, "\n")] = 0;

	if (default_branch[0] != '\0')
		strncpy(cfg.default_branch, default_branch, sizeof(cfg.default_branch));

	printf("\nYour initials are: %s\n", cfg.initials[0] ? cfg.initials : NONE_PLACEHOLDER);

	char initials[16];

	printf("Initials: ");
	fgets(initials, sizeof(initials), stdin);
	initials[strcspn(initials, "\n")] = 0;

	if (initials[0] != '\0')
		strncpy(cfg.initials, initials, sizeof(cfg.initials));

	printf("\nThe repository name is: %s\n", cfg.repository_name[0] ? cfg.repository_name : NONE_PLACEHOLDER);

	char repository_name[100];
	
	printf("Repository Name: ");
	fgets(repository_name, sizeof(repository_name), stdin);
	repository_name[strcspn(repository_name, "\n")] = 0;

	if (repository_name[0] != '\0')
		strncpy(cfg.repository_name, repository_name, sizeof(cfg.repository_name));

	save_config();
	return 0;
}

int git(char *command) {
	system(command);
	return 0;
}

int new(char ticket[8])
{
	char new_branch[255] = "";
	
	strncpy(cfg.ticket, ticket, sizeof(cfg.ticket));
	format_cfg(new_branch, sizeof(new_branch), cfg.branch_format, NONE_PLACEHOLDER, NONE_PLACEHOLDER);

	printf("Will checkout branch %s and create new branch %s.\n", cfg.default_branch, new_branch);
	if (!ask()) {
		printf(ANSI_COLOR_RED ANSI_STYLE_BOLD "Cancelled" ANSI_RESET_ALL);
		return ERR_CANCELLED;
	}

	char command[1024] = "";

	sprintf(command, "git fetch\n");
	git(command);

	sprintf(command, "git checkout %s\n", cfg.default_branch);
	git(command);

	sprintf(command, "git checkout -b %s\n", new_branch);
	git(command);

	cfg.timer = time(NULL);
	save_config();
	return 0;
}

int commit(int argc, char **argv) {
	int mins = (int)((time(NULL) - cfg.timer) / 60);

	printf("Time to commit: %i minute%s\n", mins, mins == 1 ? "" : "s");
	if (!ask()) {
		printf("Enter time (minutes): ");
		scanf("%i", &mins);
	}

    int flagc = 0;
    for (int i = 2; i < argc; i++) {
		flagc = i;
        if (argv[i][0] != '-') break;
    }

    char *flagstr = join_args(flagc, argv, 2);
    char *argstr  = join_args(argc, argv, flagc);

	char branch[255] = "";
	format_cfg(branch, sizeof(branch), cfg.branch_format, NONE_PLACEHOLDER, NONE_PLACEHOLDER);

	char message[72] = "";
	format_cfg(message, sizeof(message), cfg.commit_format, argstr, branch);
	free(argstr);

	printf("\nThe commit message will be: \"%s\"\n", message);
	if (!ask()) {
		printf(ANSI_COLOR_RED ANSI_STYLE_BOLD "Cancelled" ANSI_RESET_ALL);
		return ERR_CANCELLED;
	}

	char command[1024] = "";
	sprintf(command, "git commit %s -m \"%s\"\n", flagstr, message);
	git(command);
	free(flagstr);

	return 0;
}

int setformat(int argc, char **argv) {
	int branch = 0, commit = 0;
	for (int i = 2; i < argc; i++)
	{
		if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--branch"))
			branch = i + 1;
		else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--commit"))
			commit = i + 1;
	}
	
	int branchc = commit > branch ? commit - 1 : argc;
	int commitc = branch > commit ? branch - 1 : argc;

	if (commit)
	{
		if (commit == commitc)
			printf("%s\n", cfg.commit_format);
		else {
			char *str = join_args(commitc, argv, commit);
			strncpy(cfg.commit_format, str, sizeof(cfg.commit_format));
			free(str);
		}
	}
	if (branch) {
		if (branch == branchc)
			printf("%s\n", cfg.branch_format);
		else {
			char *str = join_args(branchc, argv, branch);
			strncpy(cfg.branch_format, str, sizeof(cfg.branch_format));
			free(str);
		}
	}
	if (!branch && !commit)
	{
		return printf("Commit Message Format: %s\nBranch Format: %s\n", cfg.commit_format, cfg.branch_format);
	}
	save_config();
	return 0;
}

void set_default_config() {
	memset(&cfg, 0, sizeof(cfg));
	cfg.version = 1;
	strncpy(cfg.branch_format, "ticket-%n", sizeof(cfg.branch_format));
	strncpy(cfg.commit_format, "[%n] %i: %m", sizeof(cfg.commit_format));
	save_config();
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
		set_default_config();
    }

	strncpy(name, argv[0], sizeof(name));
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
			if (argc > 2) {
				if (!strcmp(argv[2], "-h") || !strcmp(argv[2], "--help"))
				{
					return help_config();
				}
				else if (!strcmp(argv[2], "-r") || !strcmp(argv[2], "--reset")) {
					set_default_config();
					printf(ANSI_COLOR_YELLOW "Configuration has been reset to defaults.\n" ANSI_RESET_ALL);
					return 0;
				}
			}
			return config();
		}
		else if (!strcmp(argv[1], "new"))
		{
			printf(ANSI_COLOR_RED ANSI_STYLE_BOLD);
			
			if (argc < 3) {
				printf("Ticket number required\n");
				return help_new() | ERR_MISSING_ARG;
			}
			if (!strcmp(argv[2], "-h") || !strcmp(argv[2], "--help"))
			{
				return help_new();
			}

			int err = check_cfg();
			printf(ANSI_RESET_ALL);

			if (err) return err;
			return new(argv[2]);
		}
		else if (!strcmp(argv[1], "commit"))
		{
			printf(ANSI_COLOR_RED ANSI_STYLE_BOLD);
			if (argc < 3) {
				printf("Commit message required\n");
				return help_commit() | ERR_MISSING_ARG;
			}
			if (!strcmp(argv[2], "-h") || !strcmp(argv[2], "--help"))
			{
				return help_commit();
			}

			int err = check_cfg();
			if (!strcmp(cfg.ticket, "")) {
				printf("You are not in a ticket branch");
				err |= ERR_NO_TICKET;
			}
			printf(ANSI_RESET_ALL);

			if (err) return err;
			return commit(argc, argv);
		}
		else if (!strcmp(argv[1], "format"))
		{
			return setformat(argc, argv);
		}
	}
	printf(ANSI_COLOR_RED ANSI_STYLE_BOLD"Invalid command\n");
	return help();
}
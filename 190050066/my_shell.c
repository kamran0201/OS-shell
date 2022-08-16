#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <setjmp.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

int tokenNo = 0;
int bglist[MAX_NUM_TOKENS];
int bgi;
static sigjmp_buf env;

/* Splits the string by space and returns the array of tokens */
char **tokenize(char *line)
{
	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0;
	tokenNo = 0;
	for (i = 0; i < strlen(line); i++)
	{
		char readChar = line[i];
		if (readChar == ' ' || readChar == '\n' || readChar == '\t')
		{
			token[tokenIndex] = '\0';
			if (tokenIndex != 0)
			{
				tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0;
			}
		}
		else
		{
			token[tokenIndex++] = readChar;
		}
	}
	free(token);
	tokens[tokenNo] = NULL;
	return tokens;
}

void execfore(char **tokens)
{
	int rc = fork();
	if (rc < 0)
	{
		printf("\nFork failed");
		return;
	}
	else if (rc == 0)
	{
		if (execvp(tokens[0], tokens))
			printf("Shell : Incorrect command\n");
		exit(0);
	}
	else
	{
		waitpid(rc, NULL, 0);
		return;
	}
}

void execforell(char **tokens)
{
	int rc = fork();
	if (rc < 0)
	{
		printf("\nFork failed");
		return;
	}
	else if (rc == 0)
	{
		if (execvp(tokens[0], tokens))
			printf("Shell : Incorrect command\n");
		exit(0);
	}
	else
	{
		return;
	}
}

void execback(char **tokens)
{
	int rc = fork();
	if (rc < 0)
	{
		printf("\nFork failed");
		return;
	}
	else if (rc == 0)
	{
		setpgid(0, 0);
		if (execvp(tokens[0], tokens))
			printf("Shell : Incorrect command\n");
		exit(0);
	}
	else
	{
		for (bgi = 0; bgi < MAX_NUM_TOKENS; bgi++)
		{
			if (bglist[bgi] == 0)
			{
				bglist[bgi] = rc;
				break;
			}
		}
		return;
	}
}

void sig_handler(int signum)
{
	printf("\n");
	for (int i = 0; i < 64; i++)
		waitpid(-1, NULL, WNOHANG);
	siglongjmp(env, 10);
}

int main(int argc, char *argv[])
{
	signal(SIGINT, sig_handler);
	char line[MAX_INPUT_SIZE];
	char **tokens;
	int i, rp;
	for (i = 0; i < MAX_NUM_TOKENS; i++)
		bglist[i] = 0;
	while (1)
	{
		// for (i = 0; i < MAX_NUM_TOKENS; i++)
		// 	printf("%d ",bglist[i]);
		// printf("\n");
		sigsetjmp(env, 1);
		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		printf("$ ");
		scanf("%[^\n]", line);
		getchar();
		/* END: TAKING INPUT */
		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);

		// do whatever you want with the commands, here we just print them
		if ((rp = (int)waitpid(-1, NULL, WNOHANG)) > 0)
		{
			for (bgi = 0; bgi < MAX_NUM_TOKENS; bgi++)
			{
				if (bglist[bgi] == rp)
				{
					bglist[bgi] = 0;
					printf("Shell: Background process finished\n");
					break;
				}
			}
		}
		if (tokens[0] == NULL)
			continue;
		else if (strcmp(tokens[0], "exit") == 0)
		{
			for (bgi = 0; bgi < MAX_NUM_TOKENS; bgi++)
			{
				if (bglist[bgi] != 0)
				{
					kill(bglist[bgi], SIGKILL);
					bglist[bgi] = 0;
				}
			}
			for (i = 0; tokens[i] != NULL; i++)
				free(tokens[i]);
			free(tokens);
			exit(0);
		}
		else if (strcmp(tokens[tokenNo - 1], "&") == 0)
		{
			tokens[tokenNo - 1] = NULL;
			if (strcmp(tokens[0], "cd") == 0)
			{
				if (tokenNo != 3)
					printf("Shell: Incorrect command\n");
				else if (chdir(tokens[1]))
					printf("Shell: Incorrect command\n");
			}
			else
				execback(tokens);
		}
		else
		{
			int j = 0, subtokenNo = 0, type = 1, numTasks = 1;
			char **subtokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
			while (j < tokenNo)
			{
				if (strcmp(tokens[j], "&&") == 0)
				{
					subtokens[subtokenNo] = '\0';
					type = 1;

					if (strcmp(subtokens[0], "cd") == 0)
					{
						if (subtokenNo != 2)
							printf("Shell: Incorrect command\n");
						else if (chdir(subtokens[1]))
							printf("Shell: Incorrect command\n");
					}
					else if (strcmp(subtokens[0], "exit") == 0)
					{
						for (i = 0; subtokens[i] != NULL; i++)
							free(subtokens[i]);
						free(subtokens);
						for (i = 0; tokens[i] != NULL; i++)
							free(tokens[i]);
						free(tokens);
						exit(0);
					}
					else
						execfore(subtokens);

					subtokenNo = 0;
					if (j == tokenNo - 1)
					{
						type = 0;
						for (i = 0; subtokens[i] != NULL; i++)
							free(subtokens[i]);
						free(subtokens);
					}
				}
				else if (strcmp(tokens[j], "&&&") == 0)
				{
					subtokens[subtokenNo] = '\0';
					type = 2;
					numTasks++;
					if (strcmp(subtokens[0], "cd") == 0)
					{
						if (subtokenNo != 2)
							printf("Shell: Incorrect command\n");
						else if (chdir(subtokens[1]))
							printf("Shell: Incorrect command\n");
					}
					else if (strcmp(subtokens[0], "exit") == 0)
					{
						for (i = 0; subtokens[i] != NULL; i++)
							free(subtokens[i]);
						free(subtokens);
						for (i = 0; tokens[i] != NULL; i++)
							free(tokens[i]);
						free(tokens);
						exit(0);
					}
					else
						execforell(subtokens);

					subtokenNo = 0;
					if (j == tokenNo - 1)
					{
						type = 0;
						numTasks--;
						for (i = 0; subtokens[i] != NULL; i++)
							free(subtokens[i]);
						free(subtokens);
						for (i = 0; i < numTasks; i++)
							waitpid(-getpid(), NULL, 0);
					}
				}
				else
				{
					subtokens[subtokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
					strcpy(subtokens[subtokenNo++], tokens[j]);
				}
				j++;
			}
			if (type == 1)
			{
				subtokens[subtokenNo] = '\0';
				if (strcmp(subtokens[0], "cd") == 0)
				{
					if (subtokenNo != 2)
						printf("Shell: Incorrect command\n");
					else if (chdir(subtokens[1]))
						printf("Shell: Incorrect command\n");
				}
				else if (strcmp(subtokens[0], "exit") == 0)
				{
					for (i = 0; subtokens[i] != NULL; i++)
						free(subtokens[i]);
					free(subtokens);
					for (i = 0; tokens[i] != NULL; i++)
						free(tokens[i]);
					free(tokens);
					exit(0);
				}
				else
					execfore(subtokens);

				for (i = 0; subtokens[i] != NULL; i++)
					free(subtokens[i]);
				free(subtokens);
			}
			else if (type == 2)
			{
				subtokens[subtokenNo] = '\0';
				if (strcmp(subtokens[0], "cd") == 0)
				{
					if (subtokenNo != 2)
						printf("Shell: Incorrect command\n");
					else if (chdir(subtokens[1]))
						printf("Shell: Incorrect command\n");
				}
				else if (strcmp(subtokens[0], "exit") == 0)
				{
					for (i = 0; subtokens[i] != NULL; i++)
						free(subtokens[i]);
					free(subtokens);
					for (i = 0; tokens[i] != NULL; i++)
						free(tokens[i]);
					free(tokens);
					exit(0);
				}
				else
					execforell(subtokens);

				for (i = 0; subtokens[i] != NULL; i++)
					free(subtokens[i]);
				free(subtokens);
				for (i = 0; i < numTasks; i++)
					waitpid(-getpid(), NULL, 0);
			}
			// for (i = 0; subtokens[i] != NULL; i++)
			// 	free(subtokens[i]);
			// free(subtokens);
		}
		// Freeing the allocated memory
		for (i = 0; tokens[i] != NULL; i++)
			free(tokens[i]);
		free(tokens);
	}
	return 0;
}

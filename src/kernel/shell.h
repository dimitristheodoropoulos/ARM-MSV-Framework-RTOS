#ifndef SHELL_H
#define SHELL_H

/**
 * shell_readline() — Λήψη χαρακτήρων από το UART μέχρι το Newline.
 */
void shell_readline(char *buf, int maxlen);

/**
 * shell_process() — Εκτέλεση της εντολής.
 */
void shell_process(char *cmd);

#endif /* SHELL_H */
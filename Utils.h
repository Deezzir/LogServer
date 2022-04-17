#ifndef __UTILS_H__
#define __UTILS_H__

#include <iostream>

#define BUF_LEN 256
#define PORT 1337

/*Print Error function is responsible for*/
/*printing error messages and exiting on demand*/
void print_error(const char *msg, bool with_exit = false) {
    perror(msg);
    if (with_exit)  // exit if demanded
        exit(EXIT_FAILURE);
}
/*Clear stdin buffer*/
void clear() {
    while ((getchar()) != '\n');
}
/*Function to pause terminal screen */
void pause_screen() {
    std::cout << "Press any key to continue:";
    std::cin.get();
}
/*Function to get Y/y or N/n from user input and convert it to bool */
bool yes() {
    char answer{'\n'};
    bool value{false};

    do {
        scanf(" %c", &answer);
        switch (answer) {
            case 'n':
            case 'N':
                value = value;
                break;

            case 'y':
            case 'Y':
                value = !value;
                break;

            default:
                std::cout << "Wrong input. Try again: ";
                clear();
                break;
        }

    } while (answer != 'y' && answer != 'Y' && answer != 'N' && answer != 'n');

    answer = '\0';
    return value;
}
/*Get Int function is responsible for*/
/*receiving int input from a user and validating it*/
int get_int() {
    bool done{false};
    int value;

    done = scanf("%d", &value);
    clear();
    while (done == false) {
        std::cout << "The value entered is not a number. Try again: ";
        done = scanf(" %d", &value);
        clear();
    }

    return value;
}
/*Get Int in Range function is responsible for*/
/*receiving int input in the range from the user and validating it*/
/*Function uses get_int() as a base*/
int get_int_in_range(int min, int max) {
    bool done{false};
    int value{-1};

    do {
        value = get_int();
        if (value > max || value < min) {
            std::cout << "You entered the wrong number. Please, enter the number between " << min << " and " << max << ": ";
        } else {
            done = !done;
        }
    } while (!done);

    return value;
}

#endif  // __UTILS_H__
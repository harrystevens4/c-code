
# How it works

## Ncurses

The first thing that happens is that the program uses `setlocale(LC_all,"")` from `<locale.h>` to enable support for extended ascii. This allows the program access to displaying box drawing characters, and is how the lines are created. We also enable `cbreak()` so we can disable line buffering. This means that the tty immediately sends input to the terminal, rather then waiting for an [Enter] input. This is also where we initialise color pairs. `1` is used for the normal background and `2` for the red highlighted text. Finally, we do `curs_set(0)` to hide cursor as well as `noecho()` to make sure our characters we type are not pasted onto the screen. `keypad(stdscr, TRUE)` simply enables capturing of inputs such as the [F1] key.
Note: in order to compile properly, make sure `ncurses` is installed and use the flag `-lncursesw`

### Main window

#### Setting up variables

Before anything is displayed, the program initialises tons of variables for use later. Here is an overview of the most important ones.
- `int term_height` the height of the terminal window
- `int term_width` the width of the terminal window
> The width and height have a max and min value, and will try to retain their highest value without being to large for the screen.
- `int width` the width of the main window
- `int height` the height of the main window
> The x and y variables position the window in the centre, but they themselves refer to the top left corner of the window.
- `int x` number of characters to the left of the window
- `int y` number of characters over the top of the window
> These control cursor positioning (selection box)
- `int selected` the index in the list of units that is being hovered over by the cursor
- `int cur_selected` the index of the selected displayed (visible within the window) units. Between 0 and 13, and used to figure out which of the displayed units should be highlighted red. This was implemented due to the cursor not being on a fixed position on the screen, and being able to go to the top and bottom.
- `int offset` This variable is used to store the offset from the first element in the unit list and the first displayed element on screen. For example, if the screen scrolled down by 3, the offset would be 3.
- `const int action_width` holds a constant value for how many characters wide the right panel should be, and influences the position of the sidebar.

### pop-up box

the `void popup(const char *text)` function starts off by calculating the size of window it will need to fit the text given. It checks how many newline characters there are, and the max width of each line. It will not however let the width go above the actual width of the terminal. The next stage is to create a new window in the centre of the terminal, and draw a box around it. It then finally prints the text given to it and an `<ok>` prompt at the bottom. It then waits for any input and cleans up its variables and clears the screen.

### `void alpha_sort(char **list,int length)`

this function is used in the main window, and is a bubble sort to sort the units alphabetically (based on ascii value). It is passed the list of strings, and the length of the first dimension of the list.

## systemctl integration

the program is able to run systemctl commands through the function `void exec_cmd(const char *command, char **output)`. This function takes in a string as the command and a pointer to an output buffer.(so it can do all the memory allocation necessary). The main function calls this to run and get the output of the systemctl commands.

NAME = webserv

SRC_FILES = main.cpp Logger.cpp

SRC_DIR = src
OBJ_DIR = obj

OBJ = $(SRC_FILES:.cpp=.o)
OBJ := $(OBJ:%=$(OBJ_DIR)/%)

CC = c++
CFLAGS = -Wall -Wextra -Werror -MMD -std=c++20
INC = -I inc/

all: $(NAME)

$(NAME): $(OBJ) | Makefile
	$(CC) $(OBJ) $(CFLAGS) $(INC) -o $(NAME)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
-include $(OBJ:.o=.d)
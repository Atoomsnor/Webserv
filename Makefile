################################################################################
# Basics                                                                       #
################################################################################

NAME		= webserv
CPP			= c++
CPPFLAGS	= -Wall -Wextra -Werror -MMD -std=c++20 -static

################################################################################
# Source files                                                                 #
################################################################################

INCLUDES	= -I ./inc

SRC_DIR		= src
SRC_FILES	= main.cpp Logger.cpp

LOG_DIR		= logs

OBJ_DIR		= obj
OBJ			= $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

################################################################################
# Rules                                                                        #
################################################################################

all: $(NAME)

$(NAME): $(OBJ) Makefile
	$(CPP) $(OBJ) $(CPPFLAGS) -o $(NAME)

$(LOG_DIR):
	mkdir -p $@

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp Makefile | $(OBJ_DIR)
	$(CPP) -c $(CPPFLAGS) $(DEPFLAGS) $(INCLUDES) $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

-include $(OBJ:.o=.d)
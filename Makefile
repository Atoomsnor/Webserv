################################################################################
# Basics                                                                       #
################################################################################

NAME			= webserv
DEBUG_NAME		= webserv_debug

CPP				= c++
CPPFLAGS		= -Wall -Wextra -Werror -MMD -std=c++20 -static
DEBUG_FLAGS		= -D DEBUG_LOG=1 -g

################################################################################
# Source files                                                                 #
################################################################################

INCLUDES		= -I ./inc

SRC_DIRS		=	src/ \
					src/server/ \

SRC_FILES		=	main.cpp \
					Server.cpp ClientLoop.cpp SocketSetup.cpp ServerUtils.cpp \
					Logger.cpp Parser.cpp HTTP.cpp

LOG_DIR			= logs

OBJ_DIR			= obj/
DEBUG_OBJ_DIR	= obj_debug/

OBJ				= $(SRC_FILES:%.cpp=$(OBJ_DIR)%.o)
DEBUG_OBJ		= $(SRC_FILES:%.cpp=$(DEBUG_OBJ_DIR)%.o)

################################################################################
# Rules                                                                        #
################################################################################

all: $(NAME)

%/:
	mkdir -p $@

VPATH = $(SRC_DIRS)

################################ RELEASE #######################################

$(NAME): $(OBJ) Makefile
	$(CPP) $(OBJ) $(CPPFLAGS) -o $@

$(OBJ_DIR)%.o: %.cpp Makefile | $(OBJ_DIR) $(LOG_DIR)
	$(CPP) -c $(CPPFLAGS) $(INCLUDES) $< -o $@

################################ DEBUG #########################################

debug: $(DEBUG_NAME)
	./$(DEBUG_NAME)

$(DEBUG_NAME): $(DEBUG_OBJ) Makefile
	$(CPP) $(DEBUG_OBJ) $(CPPFLAGS) $(DEBUG_FLAGS) -o $@

$(DEBUG_OBJ_DIR)%.o: %.cpp Makefile | $(DEBUG_OBJ_DIR) $(LOG_DIR)
	$(CPP) -c $(CPPFLAGS) $(DEBUG_FLAGS) $(INCLUDES) $< -o $@

################################ COMMON ########################################

$(LOG_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(DEBUG_OBJ_DIR)

fclean: clean
	rm -f $(NAME)
	rm -f $(DEBUG_NAME)

re: fclean all

re_debug: fclean debug

.PHONY: all debug run_debug clean fclean re re_debug

-include $(OBJ:.o=.d)
-include $(DEBUG_OBJ:.o=.d)
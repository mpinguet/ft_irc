NAME = ircserv

SOURCES =	srcs/main.cpp \
			srcs/Server.cpp \
			srcs/Client.cpp \

GCC = c++

FLAGS = -Wall -Wextra -Werror -std=c++98

OBJS = ${SOURCES:.cpp=.o}

all: ${NAME}

${NAME}: ${OBJS}
	$(GCC) $(FLAGS) $(OBJS) -o $(NAME)

$(OBJS): %.o: %.cpp
	$(GCC) $(FLAGS) -c $< -o $@

clean:
	@rm -f $(OBJS)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
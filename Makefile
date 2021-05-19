infix: infix.tab.o lex.yy.o  
	gcc -g -o infix lex.yy.o infix.tab.o

scheduler: scheduler.c
	gcc -g -o scheduler scheduler.c

lex.yy.o: infix.l
	flex infix.l; gcc -c -g lex.yy.c

infix.tab.o: infix.y
	bison -d infix.y; gcc -c -g infix.tab.c

clean:
	rm -f p2 infix.output *.o infix.tab.c lex.yy.c

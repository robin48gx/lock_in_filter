



all:
	gcc  main.c -o lock_in -lm
	./lock_in > dd.txt
	#vi dd.txt
	gnuplot < lock_in.gpt

# Exercise 1
   **Development Board used: NVIDIA's Jetson Nano**
   
   **Tools used: Gcc Compiler, Vim editor, MobaXterm**
   
   **Note**: Before executing any of the codes in Question 4, run 'myscript.sh'. Linux dynamically scales CPU clock
    speed and can disable cores. This adds a non-deterministic behavior to the FIFO threads, which can be a potential
    problem for the assignments in this course. In order to avoid these issues, execute the script by using the           command "sh myscript.sh" from your **root directory** (to get to root directory, type "**sudo su**" from your         present directory) on Linux. 
    These configurations are temporary. This means that you'll have to run the script each time you reboot the           system.
      
# Folder structure:
    1) Question_3
       a) Posix_clock
          i) posix_clock.c
          ii) Makefile
    2) Question_4
       a) Sequencer
          i) question4.c
          ii) Makefile
       b) pthread
          i) pthread.c
          ii) Makefile
       c) synchronization_examples
          i) deadlock.c
          ii) pthread3.c
          iii) pthread3ok.c
          iv) Makefile
     3) Raheja_Exercise_1_Report.pdf
     4) myscript.sh
     5) README.md

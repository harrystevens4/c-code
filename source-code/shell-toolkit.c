static input_connected // 0 for no 1 for yes
int connected_to_pipe(){//returns 1 on true
}
const char *get_pipe_input_executable(){
        char command_buffer[1024];                                                                 
        int pid = 660;                                                                             
        char executable_location[1024];                                                            
        char *result;                                                                              
        FILE *output_file;                                                                         
        if ((args.number_single > 0) && (args.number_other > 0)){                                  
                if (args.single[0] == 'c'){                                                        
                        total_lines = (int)strtol(args.other[0],(char **)NULL, 10);                
                }                                                                                  
                                                                                                   
        }else{                                                                                     
                //analyse script on otherside of pipe to attempt to decipher total_lines           
                                                                                                   
                pid = getpgrp();//magic command to get the other side of the pipes pid             
                                                                                                   
                printf("attempting to analyse executable...\n");                                   
                sprintf(command_buffer,"readlink -f /proc/%d/exe",pid);//get executable location   
                printf("attempting to run %s\n",command_buffer);                                   
                output_file = popen(command_buffer,"r");                                           
                result = fgets(executable_location,1024,output_file);                              
                if (result != NULL){                                                               
                        if (output_file == NULL){                                                  
                                printf("something went wrong whilst executing %s\n",command_buffer);                                                                                                  
                        }                                                                          
                        if (feof(output_file)){                                                    
                                printf("could not deduce executable location\n");                  
                        }                                                                          
                }else{                                                                             
                        printf("no executable location found\n");                                  
                }                                                                                  
                printf("%s",executable_location);                                                  
                                                                                                   
                pclose(output_file); 
	}
}
connected_pipe_input(){//returns 1 on true (used in if statement)
}
connected_pipe_output(){//returns 1 on true
}

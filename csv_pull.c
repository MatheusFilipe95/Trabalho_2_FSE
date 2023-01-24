#include "csv_pull.h"

float csv_ref[10];

void csv_reference(){
	
		FILE* fp1 = fopen("curva_reflow.csv", "r");

 
    if (!fp1)
        printf("Arquivo com defeito\n");
 
    else {
        char buffer[1024];
 
        int row = 0;
        int column = 0;
				//printf("Valores lidos do CSV: \n");
 
        while (fgets(buffer, 1024, fp1)) {
            column = 0;
            row++;
 
            if (row == 1)
                continue;
 
            char* value = strtok(buffer, ", ");
 
            while (value) {
                if (column == 0) {
                    //printf("Time :");
                }
 
                if (column == 1) {
                    //printf("\tTemp. Ref.:");
										csv_ref[row-2] = atof(value);
										//printf("\ncsv_ref= %f", csv_ref[row-1]);

                }
 
                //printf("%s", value);
                value = strtok(NULL, ", ");
                column++;
            }
 
            //printf("\n");
        }
 
        fclose(fp1);
    }
}

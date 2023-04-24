FILE *fp;
    fp = fopen("traces.txt", "r");
    if (fp == NULL) {
        printf("Error opening file.\n");
        return 1;
    }
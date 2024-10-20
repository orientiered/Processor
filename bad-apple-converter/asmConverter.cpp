#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

const size_t FRAMES_COUNT = 6572;
const size_t WIDTH  = 96;
const size_t HEIGHT = 36;

bool writeFrame(FILE *out, const uint8_t *frame);
bool readFrame(uint8_t *frame, size_t idx);

void percentageBar(size_t value, size_t maxValue, unsigned points, long long timePassed) {
    //draw nice progress bar like
    //[###|-----] 20.0% Remaining time: 20.4 s
    printf("\r[");
    for (unsigned i = 0; i < points; i++) {
        double pointFill = double(value) / maxValue - double(i) / points;
        if (pointFill > 0)
            printf("#");
        else if (pointFill > -0.5 / points)
            printf("|");
        else
            printf("-");
    }
    printf("] %5.1f%%", double(value)/maxValue * 100);
    if (value > 0 && timePassed > 0) {
        printf(" Remaining time: %4.1f s", double(timePassed) / value * (maxValue - value) / CLOCKS_PER_SEC);
    }
    fflush(stdout);
}

bool writeFrame(FILE *out, const uint8_t *oldFrame, const uint8_t *curFrame) {
    if (oldFrame == NULL) {
        for (size_t idx = 0; idx < HEIGHT*WIDTH; idx++)
            fprintf(out, "push %u pop [%zu]\n", curFrame[idx], idx);
    } else {
        for (size_t idx = 0; idx < HEIGHT*WIDTH; idx++)
            if (oldFrame[idx] != curFrame[idx])
                fprintf(out, "push %u pop [%zu]\n", curFrame[idx], idx);
    }
    fprintf(out, "sleep 1 drawr\n");
    return true;
}

bool readFrame(uint8_t *frame, size_t idx) {
    char fileName[] = "frames-ascii/out0000.jpg.txt";
    sprintf(fileName, "frames-ascii/out%.4d.jpg.txt", idx);
    //printf("Idx = %d, name = '%s'\n", idx, fileName);
    FILE *in = fopen(fileName, "rb");
    char rowStr[WIDTH+1] = {0};
    for (size_t row = 0; row < HEIGHT; row++) {
        fread(rowStr, WIDTH + 1, 1, in);
        for (size_t col = 0; col < WIDTH; col++) {
            if (rowStr[col] == '@' || rowStr[col] == '#')
                rowStr[col] = 1;
            else
                rowStr[col] = 0;
        }
        memcpy(frame + row * WIDTH, rowStr, WIDTH);
    }
    return true;
}


int main() {
    uint8_t *curFrame = (uint8_t *) calloc(WIDTH*HEIGHT, sizeof(uint8_t)),
            *oldFrame = (uint8_t *) calloc(WIDTH*HEIGHT, sizeof(uint8_t));
    FILE *out = fopen("badApple.asm", "wb");

    readFrame(oldFrame, 1);
    writeFrame(out, NULL, oldFrame);

    clock_t convertingTime = 0, startTime = clock();

    for (size_t idx = 1; idx < FRAMES_COUNT; idx++) {
        readFrame (curFrame, idx + 1);
        // writeFrame(out, oldFrame, curFrame);
        writeFrame(out, oldFrame, curFrame);

        uint8_t *temp = oldFrame;
        oldFrame = curFrame;
        curFrame = temp;

        convertingTime = clock() - startTime;
        percentageBar(idx + 1, FRAMES_COUNT, 40, convertingTime);
    }
    printf("\n");
    fprintf(out, "hlt\n");
    free(curFrame);
    free(oldFrame);
    return 0;
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>

const size_t FRAMES_COUNT = 6572; //6572
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

inline bool writeFill(FILE *out, const long long start, const long long finish, int color) {
    assert(out);
    const int LOOP_CMD = 4;
    if (start >= 0) {
        if (finish - start >= LOOP_CMD)
            fprintf(out,
            "push %lld push %lld\npush %d\n"
            "call loopFill:\n",
            start, finish, color);
        else {
            for (size_t idx = start; idx < finish; idx++)
                fprintf(out, "push %d pop [%zu]\n", color, idx);
        }
    }
    return true;
}

bool writeFrame(FILE *out, const uint8_t *oldFrame, const uint8_t *curFrame) {
    long long seqStart = -1, seqFinish = 0;
    int color = -1;
    for (size_t idx = 0; idx < HEIGHT*WIDTH; idx++) {
        if (oldFrame == NULL) {
            writeFill(out, idx, idx+1, curFrame[idx]);
        } else if (oldFrame[idx] != curFrame[idx]) {
            if (color != curFrame[idx]) {
                writeFill(out, seqStart, seqFinish, color);
                color = curFrame[idx];
                seqStart = idx;
                seqFinish = seqStart + 1;
            } else {
                seqFinish++;
            }
        } else {
            writeFill(out, seqStart, seqFinish, color);
            seqStart = color = -1;
        }

    }
    writeFill(out, seqStart, seqFinish, color);
    fprintf(out, "call DRWAIT:\n");
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

    fprintf(out, "in\n pop rbx\n");
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
    fprintf(out,
    "\nDRWAIT:\n"
    "\ttime push rax sub\n"
    "\tpush rbx\n"
    "\tjae DRWAIT:\n"
    "\ttime pop rax\n"
    "\tdrawr\n"
    "ret\n"
    "\nloopFill:\n"
    "    pop     rcx ;color\n"
    "    pop     rdx ;last\n"
    "    pop     rex ;current\n"
    "fillStart:\n"
    "    push    rex\n"
    "    push    rdx\n"
    "    jbe     fillEnd:\n"
    "    push    rcx\n"
    "    pop     [rex]\n\n"
    "    push    rex + 1\n"
    "    pop     rex\n"
    "    jmp     fillStart:\n"
    "fillEnd:\n"
    "    ret\n");
    fclose(out);
    free(curFrame);
    free(oldFrame);
    return 0;
}


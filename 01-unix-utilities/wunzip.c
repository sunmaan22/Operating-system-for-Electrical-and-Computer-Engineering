#include <stdio.h>
#include <stdlib.h>
// RLE 압축 해제를 수행하는 함수
void perform_rle_decompression(FILE *compressed_file) {
    int run_length;
    char byte_value;
    // 압축된 데이터를 순차적으로 읽어서 원본으로 복원
    while (fread(&run_length, sizeof(int), 1, compressed_file) == 1 &&
           fread(&byte_value, sizeof(char), 1, compressed_file) == 1) {

        // 지정된 횟수만큼 문자를 반복 출력
        for (int idx = 0; idx < run_length; idx++) {
            putchar(byte_value);
        }
    }
}
// 단일 압축 파일을 처리하는 함수
int process_compressed_file(char *filename) {
    FILE *file_handle = fopen(filename, "rb");
    if (!file_handle) {
        fprintf(stderr, "wunzip: cannot open %s\n", filename);
        return 0; // 실패를 나타내는 0 반환
    }
    perform_rle_decompression(file_handle);
    fclose(file_handle);
    return 1; // 성공을 나타내는 1 반환
}
int main(int argc, char *argv[]) {
    // 명령행 인자 검증
    if (argc < 2) {
        fprintf(stdout, "wunzip: file1 [file2 ...]\n");
        return 1;
    }
    // 각 입력 파일을 순차적으로 처리
    for (int file_idx = 1; file_idx < argc; file_idx++) {
        if (!process_compressed_file(argv[file_idx])) {
            return 1; // 파일 처리 실패 시 프로그램 종료
        }
    }
    return 0;
}

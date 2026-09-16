#include <stdio.h>
#include <stdlib.h>
// Run-length encoding 압축 함수
void perform_rle_compression(FILE *source_file) {
    char ch, prev_ch;
    int run_length;
    // 첫 번째 바이트 처리
    if (fread(&prev_ch, sizeof(char), 1, source_file) != 1) {
        return; // 빈 파일인 경우 종료
    }
    run_length = 1;
    // 나머지 바이트들 순차 처리
    while (fread(&ch, sizeof(char), 1, source_file) == 1) {
        if (ch == prev_ch) {
            run_length++;
        } else {
            // 현재 run의 결과를 바이너리로 출력
            fwrite(&run_length, sizeof(int), 1, stdout);
            fwrite(&prev_ch, sizeof(char), 1, stdout);

            prev_ch = ch;
            run_length = 1;
        }
    }
    // 최종 run 출력
    fwrite(&run_length, sizeof(int), 1, stdout);
    fwrite(&prev_ch, sizeof(char), 1, stdout);
}
// 여러 파일을 하나로 병합하는 함수
FILE* merge_input_files(int file_count, char **file_names) {
    FILE *merged = fopen("merged_temp.dat", "w+b");
    if (!merged) {
        perror("Failed to create merge file");
        return NULL;
    }
    for (int idx = 1; idx < file_count; idx++) {
        FILE *current_file = fopen(file_names[idx], "rb");
        if (!current_file) {
            fprintf(stdout, "wzip: cannot open %s\n", file_names[idx]);
            fclose(merged);
            return NULL;
        }
        // 현재 파일의 모든 데이터를 병합 파일로 복사
        char data_chunk[2048];
        size_t chunk_size;
        while ((chunk_size = fread(data_chunk, 1, sizeof(data_chunk), current_file)) > 0) {
            fwrite(data_chunk, 1, chunk_size, merged);
        }
        fclose(current_file);
    }
    // 파일 포인터를 처음으로 되돌림
    rewind(merged);
    return merged;
}
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stdout, "wzip: file1 [file2 ...]\n");
        return 1;
    }
    // 입력 파일들을 병합
    FILE *combined_input = merge_input_files(argc, argv);
    if (!combined_input) {
        return 1;
    }
    // 병합된 파일에 대해 RLE 압축 수행
    perform_rle_compression(combined_input);

    fclose(combined_input);

    // 임시 병합 파일 제거
    remove("merged_temp.dat");
    return 0;
}

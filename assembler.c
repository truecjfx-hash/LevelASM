// ======================
// CJFX 汇编器 (Assembler)
// ======================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

// 指令定义
typedef struct {
    char name[16];      // 指令助记符
    uint8_t opcode;     // 操作码
    int param_count;    // 参数数量
    int param_bytes;    // 参数总字节数
} InstructionDef;

// 指令表 (根据你的文档)
InstructionDef instruction_table[] = {
    {"HALT", 0x00, 0, 0},
    {"NOP", 0x01, 0, 0},
    {"LDX", 0x02, 1, 1},
    {"LDY", 0x03, 1, 1},
    {"LDT", 0x04, 1, 1},
    {"LDL", 0x05, 1, 1},
    {"LDH", 0x06, 1, 1},
    {"DRAW", 0x07, 1, 1},
    {"READ", 0x08, 2, 2},
    {"SET", 0x09, 1, 1},
    {"ADD", 0x0A, 1, 1},
    {"OR", 0x0B, 1, 1},
    {"AND", 0x0C, 1, 1},
    {"XOR", 0x0D, 1, 1},
    {"LDR", 0x0E, 1, 1},
    {"RDR", 0x0F, 1, 1},
    {"SUB", 0x10, 1, 1},
    {"SHL", 0x11, 1, 1},
    {"SHR", 0x12, 1, 1},
    {"INC", 0x13, 0, 0},
    {"DEC", 0x14, 0, 0},
    {"JMPF", 0x15, 1, 1},
    {"JMPB", 0x16, 1, 1},
    {"JRF", 0x17, 2, 2},
    {"JRB", 0x18, 2, 2},
    {"GETR", 0x1C, 0, 0},
    {"LDI", 0x1D, 1, 1},
    {"SETR", 0x1E, 1, 1},
    {"APPLY", 0x1F, 0, 0},
    {NULL, 0, 0, 0}  // 结束标记
};

// 标签表 (用于跳转)
typedef struct {
    char name[64];
    uint16_t address;
} LabelEntry;

LabelEntry label_table[256];
int label_count = 0;

// ======================
// 工具函数
// ======================

// 去除字符串两端空白
void trim(char *str) {
    int i = 0, j = strlen(str) - 1;
    while (isspace(str[i])) i++;
    while (j >= i && isspace(str[j])) j--;
    str[j + 1] = '\0';
    if (i > 0) memmove(str, str + i, strlen(str + i) + 1);
}

// 去除注释 (分号后面的内容)
void remove_comment(char *line) {
    char *comment = strchr(line, ';');
    if (comment) *comment = '\0';
}

// 转换数字字符串 (支持十进制、十六进制0x、二进制0b)
uint8_t parse_number(const char *str) {
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        // 十六进制
        return (uint8_t)strtol(str + 2, NULL, 16);
    } else if (str[0] == '0' && (str[1] == 'b' || str[1] == 'B')) {
        // 二进制
        return (uint8_t)strtol(str + 2, NULL, 2);
    } else if (str[0] == 'R' || str[0] == 'r') {
        // 寄存器引用 R0-R7
        return (uint8_t)atoi(str + 1);
    } else {
        // 十进制
        return (uint8_t)atoi(str);
    }
}

// 查找指令定义
InstructionDef* find_instruction(const char *name) {
    for (int i = 0; instruction_table[i].name != NULL; i++) {
        if (strcasecmp(instruction_table[i].name, name) == 0) {
            return &instruction_table[i];
        }
    }
    return NULL;
}

// 查找标签地址
int find_label_address(const char *name) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(label_table[i].name, name) == 0) {
            return label_table[i].address;
        }
    }
    return -1;
}

// ======================
// 两遍汇编器
// ======================

// 第一遍：收集标签
void first_pass(FILE *src, uint16_t *code_size) {
    char line[256];
    uint16_t address = 0;
    
    rewind(src);
    
    while (fgets(line, sizeof(line), src)) {
        trim(line);
        remove_comment(line);
        if (line[0] == '\0') continue;  // 空行
        
        // 检查是否是标签定义 (以冒号结尾)
        int len = strlen(line);
        if (line[len - 1] == ':') {
            line[len - 1] = '\0';
            trim(line);
            
            // 保存标签
            if (label_count < 256) {
                strcpy(label_table[label_count].name, line);
                label_table[label_count].address = address;
                label_count++;
                printf("发现标签: %s @ 0x%04X\n", line, address);
            }
            continue;
        }
        
        // 解析指令
        char opcode_str[32];
        if (sscanf(line, "%31s", opcode_str) == 1) {
            InstructionDef *def = find_instruction(opcode_str);
            if (def) {
                address += 1 + def->param_bytes;  // 操作码 + 参数
            }
        }
    }
    
    *code_size = address;
}

// 第二遍：生成字节码
uint8_t* second_pass(FILE *src, uint16_t code_size) {
    char line[256];
    uint16_t address = 0;
    uint8_t *bytecode = malloc(code_size);
    
    if (!bytecode) return NULL;
    
    rewind(src);
    
    while (fgets(line, sizeof(line), src)) {
        trim(line);
        remove_comment(line);
        if (line[0] == '\0') continue;
        
        // 跳过标签定义行
        if (line[strlen(line) - 1] == ':') continue;
        
        // 解析指令和参数
        char opcode_str[32];
        char param1[32] = {0}, param2[32] = {0};
        int params = sscanf(line, "%31s %31s %31s", opcode_str, param1, param2);
        
        InstructionDef *def = find_instruction(opcode_str);
        if (!def) {
            printf("错误: 未知指令 '%s'\n", opcode_str);
            free(bytecode);
            return NULL;
        }
        
        // 检查参数数量
        if (params - 1 != def->param_count) {
            printf("错误: 指令 %s 需要 %d 个参数\n", def->name, def->param_count);
            free(bytecode);
            return NULL;
        }
        
        // 写入操作码
        bytecode[address++] = def->opcode;
        
        // 处理参数
        for (int i = 0; i < def->param_count; i++) {
            char *param = (i == 0) ? param1 : param2;
            
            // 检查是否是标签引用（用于跳转指令）
            int label_addr = -1;
            if (def->opcode == 0x15 || def->opcode == 0x16 || 
                def->opcode == 0x17 || def->opcode == 0x18) {
                label_addr = find_label_address(param);
            }
            
            if (label_addr != -1) {
                // 计算跳转偏移量
                int offset;
                if (def->opcode == 0x15 || def->opcode == 0x17) {
                    // 向前跳转：目标地址 - 当前指令结束地址
                    offset = label_addr - (address + def->param_bytes + 1);
                    if (offset < 0) {
                        printf("错误: 标签 %s 在 JMPF/JRF 后面\n", param);
                        free(bytecode);
                        return NULL;
                    }
                } else {
                    // 向后跳转：当前指令结束地址 - 目标地址
                    offset = (address + def->param_bytes + 1) - label_addr;
                    if (offset < 0) {
                        printf("错误: 标签 %s 在 JMPB/JRB 前面\n", param);
                        free(bytecode);
                        return NULL;
                    }
                }
                
                if (offset > 255) {
                    printf("错误: 跳转偏移量太大 (0x%X > 255)\n", offset);
                    free(bytecode);
                    return NULL;
                }
                
                bytecode[address++] = offset;
            } else {
                // 普通数值参数
                bytecode[address++] = parse_number(param);
            }
        }
    }
    
    return bytecode;
}

// ======================
// 主汇编函数
// ======================

int assemble(const char *input_file, const char *output_file, int output_format) {
    FILE *src = fopen(input_file, "r");
    if (!src) {
        printf("无法打开源文件: %s\n", input_file);
        return -1;
    }
    
    printf("开始汇编: %s\n", input_file);
    
    // 第一遍：收集标签，计算代码大小
    uint16_t code_size = 0;
    first_pass(src, &code_size);
    printf("代码大小: %d 字节\n", code_size);
    
    // 第二遍：生成字节码
    uint8_t *bytecode = second_pass(src, code_size);
    fclose(src);
    
    if (!bytecode) {
        printf("汇编失败\n");
        return -1;
    }
    
    // 输出结果
    if (output_format == 0) {
        // 二进制文件
        FILE *bin = fopen(output_file, "wb");
        if (bin) {
            fwrite(bytecode, 1, code_size, bin);
            fclose(bin);
            printf("生成二进制文件: %s\n", output_file);
        }
    } else if (output_format == 1) {
        // C头文件
        FILE *header = fopen(output_file, "w");
        if (header) {
            fprintf(header, "// ==================================\n");
            fprintf(header, "// CJFX 字节码 - 自动生成\n");
            fprintf(header, "// 源文件: %s\n", input_file);
            fprintf(header, "// ==================================\n\n");
            fprintf(header, "const unsigned char PROGRAM[] = {\n");
            
            for (int i = 0; i < code_size; i++) {
                if (i % 16 == 0) fprintf(header, "    ");
                fprintf(header, "0x%02X", bytecode[i]);
                if (i < code_size - 1) fprintf(header, ", ");
                if (i % 16 == 15) fprintf(header, "\n");
            }
            
            if (code_size % 16 != 0) fprintf(header, "\n");
            fprintf(header, "};\n\n");
            fclose(header);
            printf("生成C头文件: %s\n", output_file);
        }
    }
    
    // 显示字节码
    printf("\n字节码内容:\n");
    for (int i = 0; i < code_size; i++) {
        if (i % 16 == 0) printf("%04X: ", i);
        printf("%02X ", bytecode[i]);
        if (i % 16 == 15) printf("\n");
    }
    if (code_size % 16 != 0) printf("\n");
    
    free(bytecode);
    printf("\n汇编成功完成!\n");
    return 0;
}

// ======================
// 图形化汇编器界面
// ======================

#ifdef _WIN32
#include <windows.h>
#include <conio.h>

void gui_assembler() {
    char input_file[MAX_PATH] = {0};
    char output_file[MAX_PATH] = {0};
    int format = 1;  // 默认生成C头文件
    
    printf("╔══════════════════════════════════════╗\n");
    printf("║       CJFX 汇编器 v1.0               ║\n");
    printf("║       程序化地图生成工具             ║\n");
    printf("╚══════════════════════════════════════╝\n\n");
    
    // 获取输入文件
    printf("请输入汇编源文件 (.cjfxasm): ");
    gets(input_file);
    
    if (strlen(input_file) == 0) {
        printf("使用默认文件: test.cjfxasm\n");
        strcpy(input_file, "test.cjfxasm");
    }
    
    // 自动生成输出文件名
    char base_name[256];
    strcpy(base_name, input_file);
    char *dot = strrchr(base_name, '.');
    if (dot) *dot = '\0';
    
    sprintf(output_file, "%s.bin", base_name);
    char c_header[256];
    sprintf(c_header, "%s.h", base_name);
    
    // 选择输出格式
    printf("\n选择输出格式:\n");
    printf("1. C头文件 (*.h) - 推荐\n");
    printf("2. 二进制文件 (*.bin)\n");
    printf("选择 [1-2]: ");
    
    char choice = getchar();
    getchar();  // 清除换行符
    
    if (choice == '2') {
        format = 0;
        printf("输出文件: %s\n", output_file);
    } else {
        printf("输出文件: %s\n", c_header);
        strcpy(output_file, c_header);
    }
    
    // 开始汇编
    printf("\n开始汇编...\n");
    assemble(input_file, output_file, format);
    
    printf("\n按任意键继续...");
    getch();
}

#else
// Linux/macOS 版本
void gui_assembler() {
    printf("\033[1;36m");  // 青色
    printf("╔══════════════════════════════════════╗\n");
    printf("║       CJFX 汇编器 v1.0               ║\n");
    printf("║       程序化地图生成工具             ║\n");
    printf("╚══════════════════════════════════════╝\n");
    printf("\033[0m\n");
    
    char input_file[256];
    char output_file[256];
    
    printf("输入汇编源文件: ");
    scanf("%255s", input_file);
    
    // 简单处理
    assemble(input_file, "output.bin", 0);
    printf("完成! 按Enter键退出...");
    getchar(); getchar();
}
#endif

// ======================
// 主程序
// ======================

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // 没有参数，启动GUI模式
        gui_assembler();
    } else if (argc == 3) {
        // 命令行模式: assembler input.vmasm output.vmb
        assemble(argv[1], argv[2], 0);
    } else if (argc == 4 && strcmp(argv[3], "-c") == 0) {
        // 生成C头文件
        assemble(argv[1], argv[2], 1);
    } else {
        printf("使用说明:\n");
        printf("  1. 图形模式: %s\n", argv[0]);
        printf("  2. 汇编为二进制: %s input.cjfxasm output.bin\n", argv[0]);
        printf("  3. 汇编为C头文件: %s input.cjfxasm output.h -c\n", argv[0]);
        printf("\n支持的数字格式:\n");
        printf("  十进制: 123\n");
        printf("  十六进制: 0x7B\n");
        printf("  二进制: 0b01111011\n");
        printf("  寄存器: R0-R7\n");
        printf("\n汇编语言特性:\n");
        printf("  - 标签: LABEL:\n");
        printf("  - 注释: ; 这是注释\n");
        printf("  - 跳转: JMPF LABEL, JRB R0 5 LABEL\n");
    }
    
    return 0;
}

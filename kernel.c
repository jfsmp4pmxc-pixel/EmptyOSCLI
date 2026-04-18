#define VIDEO_MEM 0xB8000
#define RED 0x0C
#define GREEN 0x0A
#define YELLOW 0x0E
#define WHITE 0x0F
#define CYAN 0x0B
#define GRAY 0x07

unsigned char disk_buf[512]; 
unsigned int cursor_pos = 0;
char current_path[32] = "/";

// ==========================================
// 1. RAM DISK DRIVER
// ==========================================
unsigned char ram_disk[102 * 512]; 

int read_sector(unsigned int lba, unsigned char* buffer) {
    if (lba > 101) return 0;
    for(int i = 0; i < 512; i++) buffer[i] = ram_disk[lba * 512 + i];
    return 1;
}

int write_sector(unsigned int lba, unsigned char* buffer) {
    if (lba > 101) return 0;
    for(int i = 0; i < 512; i++) ram_disk[lba * 512 + i] = buffer[i];
    return 1;
}

// ==========================================
// 2. HELPER & I/O
// ==========================================
void outb(unsigned short port, unsigned char val) { asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port)); }
unsigned char inb(unsigned short port) { unsigned char ret; asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret; }
void io_wait() { outb(0x80, 0); }

int str_len(char* s) { int i=0; while(s[i]) i++; return i; }
int str_cmp(char* s1, char* s2) { int i=0; while(s1[i] && s2[i] && s1[i]==s2[i]) i++; return s1[i] - s2[i]; }

// ==========================================
// 3. VGA & GIAO DIỆN
// ==========================================
void update_cursor(int pos) { outb(0x3D4, 14); outb(0x3D5, pos >> 8); outb(0x3D4, 15); outb(0x3D5, pos & 0xFF); }
void scroll() { unsigned short* mem = (unsigned short*)VIDEO_MEM; for (int i = 0; i < 80 * 24; i++) mem[i] = mem[i + 80]; for (int i = 80 * 24; i < 80 * 25; i++) mem[i] = (unsigned short)' ' | (GRAY << 8); cursor_pos = 80 * 24; }
void put_char(char c, unsigned char color) { unsigned short* mem = (unsigned short*)VIDEO_MEM; if (c == '\n') cursor_pos = (cursor_pos / 80 + 1) * 80; else mem[cursor_pos++] = (unsigned short)c | (unsigned short)(color << 8); if (cursor_pos >= 80 * 25) scroll(); update_cursor(cursor_pos); }
void print(char* str, unsigned char color) { for (int i = 0; str[i] != '\0'; i++) put_char(str[i], color); }
void cls() { unsigned short* mem = (unsigned short*)VIDEO_MEM; for (int i = 0; i < 80 * 25; i++) mem[i] = (unsigned short)' ' | (GRAY << 8); cursor_pos = 0; update_cursor(0); }

void print_info() {
    print("CLI mode\n", GREEN);
    print("\n      --- EmptyOS v0.4 - Single Level FS ---\n\n", WHITE); 
}

// ==========================================
// 4. HỆ THỐNG FILE (EFS v2)
// ==========================================
struct FileEntry {
    char name[12];
    unsigned char present;
    unsigned char sector; 
    unsigned short size;  
    char padding[16];     
};

void do_format() { 
    for(int i=0; i<102*512; i++) ram_disk[i] = 0;
    print("Format Success! All data cleared.\n", GREEN); 
}

void do_ls() { 
    read_sector(100, disk_buf); 
    struct FileEntry* entries = (struct FileEntry*)disk_buf;
    print("NAME         SECTOR  SIZE\n---------------------------\n", CYAN); 
    int found = 0; 
    for(int i = 0; i < 16; i++) { 
        if(entries[i].present) { 
            found = 1; 
            print(entries[i].name, WHITE); 
            for(int j=0; j < 13 - str_len(entries[i].name); j++) put_char(' ', WHITE); 
            
            // In Sector
            put_char(entries[i].sector/100 + '0', YELLOW);
            put_char((entries[i].sector/10)%10 + '0', YELLOW);
            put_char(entries[i].sector%10 + '0', YELLOW);
            print("     ", WHITE);
            
            // In Size
            int s = entries[i].size;
            put_char((s/100)%10 + '0', GREEN);
            put_char((s/10)%10 + '0', GREEN);
            put_char(s%10 + '0', GREEN);
            print(" B\n", WHITE); 
        } 
    } 
    if(!found) print("Empty directory.\n", YELLOW); 
}

void do_touch(char* fname) { 
    if(!fname[0]) { print("Usage: touch <filename>\n", RED); return; } 
    read_sector(100, disk_buf); 
    struct FileEntry* entries = (struct FileEntry*)disk_buf;
    for(int i = 0; i < 16; i++) { 
        if(!entries[i].present) { 
            for(int j=0; j<12; j++) entries[i].name[j] = 0; // Xóa tên cũ
            for(int j=0; j<12 && fname[j] && fname[j] != ' '; j++) entries[i].name[j] = fname[j]; 
            entries[i].present = 1; 
            entries[i].sector = 101 + i; 
            entries[i].size = 0; 
            write_sector(100, disk_buf); 
            print("File created.\n", GREEN); 
            return; 
        } 
    } 
    print("Disk full.\n", RED); 
}

void do_echo(char* args) {
    char content[256]; char filename[12];
    for(int i=0; i<256; i++) content[i] = 0;
    for(int i=0; i<12; i++) filename[i] = 0;

    int i = 0, c_idx = 0;
    // Tách nội dung trước dấu '>'
    while(args[i] && args[i] != '>') content[c_idx++] = args[i++];
    if (c_idx > 0 && content[c_idx-1] == ' ') content[c_idx-1] = 0; 

    if (args[i] == '>') {
        i++; while(args[i] == ' ') i++; // Bỏ qua khoảng trắng
        int f_idx = 0;
        while(args[i] && args[i] != ' ' && f_idx < 11) filename[f_idx++] = args[i++];
    } else {
        // Chỉ in ra màn hình nếu không có '>'
        print(content, WHITE); print("\n", WHITE); return;
    }

    if(filename[0] == 0) { print("Error: No filename specified.\n", RED); return; }

    read_sector(100, disk_buf);
    struct FileEntry* entries = (struct FileEntry*)disk_buf;
    for(int j=0; j<16; j++) {
        if(entries[j].present && str_cmp(entries[j].name, filename) == 0) {
            unsigned char t_buf[512];
            for(int k=0; k<512; k++) t_buf[k] = 0;
            for(int k=0; content[k] && k<512; k++) t_buf[k] = content[k];
            
            write_sector(entries[j].sector, t_buf);
            entries[j].size = str_len(content);
            write_sector(100, disk_buf);
            print("Data written to file.\n", GREEN);
            return;
        }
    }
    print("Error: File not found.\n", RED);
}

void do_cat(char* fname) {
    if(!fname[0]) { print("Usage: cat <filename>\n", RED); return; }
    read_sector(100, disk_buf);
    struct FileEntry* entries = (struct FileEntry*)disk_buf;
    for(int i=0; i<16; i++) {
        if(entries[i].present && str_cmp(entries[i].name, fname) == 0) {
            if(entries[i].size == 0) { print("(Empty File)\n", GRAY); return; }
            unsigned char t_buf[512];
            read_sector(entries[i].sector, t_buf);
            for(int k=0; k<entries[i].size; k++) put_char(t_buf[k], WHITE);
            print("\n", WHITE);
            return;
        }
    }
    print("Error: File not found.\n", RED);
}

// ==========================================
// 5. SHELL
// ==========================================
int is_cmd(char* in, char* tgt) { int i = 0; while(tgt[i]) { if(in[i] != tgt[i]) return 0; i++; } return (in[i] == ' ' || in[i] == '\0'); }

char get_ascii(unsigned char sc) { 
    static char map[] = {0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,'q','w','e','r','t','y','u','i','o','p','[',']',0,0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '}; 
    return (sc < 58) ? map[sc] : 0; 
}

void print_prompt() {
    print("root@EmptyOS", GREEN); print(":", WHITE); 
    print(current_path, CYAN); print("# ", WHITE);
}

void kmain() {
    cls(); print_info(); 
    do_format(); // Khởi tạo RamDisk sạch sẽ
    
    print_prompt();
    char cmd[128]; int i = 0; 
    unsigned int start_line_pos = cursor_pos; 

    while(1) {
        while (!(inb(0x64) & 0x01)) io_wait(); 
        unsigned char sc = inb(0x60);

        if (sc < 0x80) { 
            if (sc == 0x1C) { // ENTER
                cmd[i] = '\0'; put_char('\n', WHITE);
                if (i > 0) {
                    if (is_cmd(cmd, "cls")) cls();
                    else if (is_cmd(cmd, "help")) print("Cmds: cls, format, ls, touch, echo, cat\n", WHITE);
                    else if (is_cmd(cmd, "format")) do_format();
                    else if (is_cmd(cmd, "ls")) do_ls();
                    else if (is_cmd(cmd, "touch")) do_touch(cmd + 6);
                    else if (is_cmd(cmd, "echo")) do_echo(cmd + 5);
                    else if (is_cmd(cmd, "cat")) do_cat(cmd + 4);
                    else print("Command not found.\n", RED);
                }
                i = 0; print_prompt(); start_line_pos = cursor_pos; 
            } 
            else if (sc == 0x0E) { // BACKSPACE
                if (i > 0 && cursor_pos > start_line_pos) { 
                    i--; cursor_pos--; 
                    ((unsigned short*)VIDEO_MEM)[cursor_pos] = ' ' | (GRAY << 8); 
                    update_cursor(cursor_pos); 
                    cmd[i] = '\0';
                }
            } 
            else { // Phím thường
                char c = get_ascii(sc);
                if (c && i < 127) { 
                    if(sc == 0x33) c = '>'; // Bật phím '>' khi gõ dấu chấm hoặc phím tương đương (tạm map cứng để dễ test)
                    cmd[i++] = c; cmd[i] = '\0'; 
                    put_char(c, WHITE);
                }
            }
        }
    }
}
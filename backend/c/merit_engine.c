#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_STUDENTS 2000
#define NAME_LEN 100
#define ROLL_LEN 40
#define BRANCH_LEN 20
#define CSV_PATH "../../data/students.csv"

typedef struct {
    char name[NAME_LEN];
    char roll[ROLL_LEN];
    char branch[BRANCH_LEN];
    double marks;
} Student;

static Student students[MAX_STUDENTS];
static int count = 0;

static void json_string(const char *s) {
    putchar('"');
    for (; *s; s++) {
        if (*s == '"' || *s == '\\') { putchar('\\'); putchar(*s); }
        else if (*s == '\n') printf("\\n");
        else if ((unsigned char)*s >= 32) putchar(*s);
    }
    putchar('"');
}

static int load_csv(void) {
    FILE *fp = fopen(CSV_PATH, "r");
    char line[512];
    count = 0;
    if (!fp) return 0;
    if (fgets(line, sizeof line, fp) == NULL) { fclose(fp); return 1; }
    while (fgets(line, sizeof line, fp) && count < MAX_STUDENTS) {
        char *name = strtok(line, ",");
        char *roll = strtok(NULL, ",");
        char *branch = strtok(NULL, ",");
        char *marks = strtok(NULL, ",\r\n");
        if (!name || !roll || !branch || !marks) continue;
        snprintf(students[count].name, NAME_LEN, "%s", name);
        snprintf(students[count].roll, ROLL_LEN, "%s", roll);
        snprintf(students[count].branch, BRANCH_LEN, "%s", branch);
        students[count].marks = atof(marks);
        count++;
    }
    fclose(fp);
    return 1;
}

static int save_csv(void) {
    FILE *fp = fopen(CSV_PATH, "w");
    if (!fp) return 0;
    fprintf(fp, "name,roll,branch,marks\n");
    for (int i = 0; i < count; i++)
        fprintf(fp, "%s,%s,%s,%.2f\n", students[i].name, students[i].roll,
                students[i].branch, students[i].marks);
    fclose(fp);
    return 1;
}

static void swap_student(Student *a, Student *b) {
    Student t = *a; *a = *b; *b = t;
}

/* Higher marks first; ties are ordered by roll number for consistent output. */
static int comes_before(Student a, Student b) {
    if (a.marks > b.marks) return 1;
    if (a.marks < b.marks) return 0;
    return strcmp(a.roll, b.roll) < 0;
}

static long long comparisons = 0;
static void merge(Student a[], Student temp[], int left, int mid, int right) {
    int i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right) {
        comparisons++;
        if (comes_before(a[i], a[j])) temp[k++] = a[i++];
        else temp[k++] = a[j++];
    }
    while (i <= mid) temp[k++] = a[i++];
    while (j <= right) temp[k++] = a[j++];
    for (i = left; i <= right; i++) a[i] = temp[i];
}
static void merge_sort_rec(Student a[], Student temp[], int left, int right) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    merge_sort_rec(a, temp, left, mid);
    merge_sort_rec(a, temp, mid + 1, right);
    merge(a, temp, left, mid, right);
}
static void merge_sort(Student a[], int n) {
    Student *temp = (Student *)malloc(sizeof(Student) * (size_t)n);
    comparisons = 0;
    if (!temp || n < 2) { free(temp); return; }
    merge_sort_rec(a, temp, 0, n - 1);
    free(temp);
}
static int partition(Student a[], int low, int high) {
    Student pivot = a[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        comparisons++;
        if (comes_before(a[j], pivot)) {
            i++; swap_student(&a[i], &a[j]);
        }
    }
    swap_student(&a[i + 1], &a[high]);
    return i + 1;
}
static void quick_sort_rec(Student a[], int low, int high) {
    if (low < high) {
        int p = partition(a, low, high);
        quick_sort_rec(a, low, p - 1);
        quick_sort_rec(a, p + 1, high);
    }
}
static void quick_sort(Student a[], int n) {
    comparisons = 0;
    if (n > 1) quick_sort_rec(a, 0, n - 1);
}

static void print_students_json(Student a[], int n, const char *algorithm) {
    printf("{\"ok\":true,\"algorithm\":");
    json_string(algorithm);
    printf(",\"count\":%d,\"students\":[", n);
    for (int i = 0; i < n; i++) {
        if (i) putchar(',');
        printf("{\"name\":"); json_string(a[i].name);
        printf(",\"roll\":"); json_string(a[i].roll);
        printf(",\"branch\":"); json_string(a[i].branch);
        printf(",\"marks\":%.2f,\"rank\":%d}", a[i].marks, i + 1);
    }
    printf("]}\n");
}

static int valid_field(const char *s) {
    return s && *s && !strchr(s, ',') && !strchr(s, '\n') && !strchr(s, '\r');
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("{\"ok\":false,\"error\":\"Usage: merit_engine list|add|sort|analytics|benchmark\"}\n");
        return 1;
    }
    if (!load_csv()) {
        printf("{\"ok\":false,\"error\":\"Could not open data/students.csv. Start the API from backend/api so the relative path resolves.\"}\n");
        return 1;
    }

    if (strcmp(argv[1], "list") == 0) {
        print_students_json(students, count, "none");
    } else if (strcmp(argv[1], "add") == 0) {
        if (argc != 6 || !valid_field(argv[2]) || !valid_field(argv[3]) ||
            !valid_field(argv[4])) {
            printf("{\"ok\":false,\"error\":\"Provide name, roll, branch, marks. Commas are not allowed in text fields.\"}\n");
            return 1;
        }
        char *end = NULL;
        double marks = strtod(argv[5], &end);
        if (!end || *end || marks < 0 || marks > 100 || count >= MAX_STUDENTS) {
            printf("{\"ok\":false,\"error\":\"Marks must be 0-100 and the student limit must not be exceeded.\"}\n");
            return 1;
        }
        for (int i = 0; i < count; i++) {
            if (strcmp(students[i].roll, argv[3]) == 0) {
                printf("{\"ok\":false,\"error\":\"Roll number already exists.\"}\n");
                return 1;
            }
        }
        snprintf(students[count].name, NAME_LEN, "%s", argv[2]);
        snprintf(students[count].roll, ROLL_LEN, "%s", argv[3]);
        snprintf(students[count].branch, BRANCH_LEN, "%s", argv[4]);
        students[count].marks = marks;
        count++;
        if (!save_csv()) {
            printf("{\"ok\":false,\"error\":\"Could not save CSV file.\"}\n");
            return 1;
        }
        print_students_json(students, count, "none");
    } else if (strcmp(argv[1], "sort") == 0) {
        if (argc < 3) {
            printf("{\"ok\":false,\"error\":\"Choose merge or quick.\"}\n"); return 1;
        }
        if (strcmp(argv[2], "merge") == 0) {
            merge_sort(students, count);
            print_students_json(students, count, "Merge Sort");
        } else if (strcmp(argv[2], "quick") == 0) {
            quick_sort(students, count);
            print_students_json(students, count, "Quick Sort");
        } else {
            printf("{\"ok\":false,\"error\":\"Unknown algorithm.\"}\n"); return 1;
        }
    } else if (strcmp(argv[1], "analytics") == 0) {
        double sum = 0, highest = 0, passed = 0;
        for (int i = 0; i < count; i++) {
            sum += students[i].marks;
            if (students[i].marks > highest || i == 0) highest = students[i].marks;
            if (students[i].marks >= 40) passed++;
        }
        printf("{\"ok\":true,\"total\":%d,\"average\":%.2f,\"highest\":%.2f,\"passPercent\":%.2f}\n",
               count, count ? sum / count : 0, highest, count ? passed * 100.0 / count : 0);
    } else if (strcmp(argv[1], "benchmark") == 0) {
        Student *a = (Student *)malloc(sizeof(Student) * (size_t)(count ? count : 1));
        Student *b = (Student *)malloc(sizeof(Student) * (size_t)(count ? count : 1));
        if (!a || !b) { free(a); free(b); printf("{\"ok\":false,\"error\":\"Out of memory.\"}\n"); return 1; }
        memcpy(a, students, sizeof(Student) * (size_t)count);
        memcpy(b, students, sizeof(Student) * (size_t)count);
        clock_t start = clock(); merge_sort(a, count); double merge_ms = 1000.0 * (clock() - start) / CLOCKS_PER_SEC; long long merge_cmp = comparisons;
        start = clock(); quick_sort(b, count); double quick_ms = 1000.0 * (clock() - start) / CLOCKS_PER_SEC; long long quick_cmp = comparisons;
        printf("{\"ok\":true,\"count\":%d,\"merge\":{\"milliseconds\":%.4f,\"comparisons\":%lld},\"quick\":{\"milliseconds\":%.4f,\"comparisons\":%lld},\"note\":\"Timing is machine- and dataset-dependent; comparisons are generally more stable for classroom comparison.\"}\n",
               count, merge_ms, merge_cmp, quick_ms, quick_cmp);
        free(a); free(b);
    } else {
        printf("{\"ok\":false,\"error\":\"Unknown command.\"}\n"); return 1;
    }
    return 0;
}

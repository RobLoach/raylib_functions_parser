/**********************************************************************************************

    raylib_functions_parser - raylib header parser to get functions data

    This parser scans raylib.h for functions that start with RLAPI and stores functions data
    into a structure separting all its parts

    WARNING: Be careful with functions that split parameters into several lines, it breaks the process!

    LICENSE: zlib/libpng

    raylib is licensed under an unmodified zlib/libpng license, which is an OSI-certified,
    BSD-like license that allows static linking with closed source software:

    Copyright (c) 2021 Ramon Santamaria (@raysan5)

**********************************************************************************************/

#include <stdlib.h>             // Required for: malloc(), calloc(), realloc(), free()
#include <stdio.h>              // Required for: printf(), fopen(), fseek(), ftell(), fread(), fclose()

#define MAX_FUNC_LINES   512    // Maximum number of functions
#define MAX_LINE_LENGTH  512    // Maximum length of function line (including comments)

#define FUNCTION_LINE_DECLARATION    "RLAPI"     // Functions start with this declaration

// Function info data
typedef struct FunctionInfo {
    char retType[32];           // Return value type
    char name[64];              // Return value name
    int paramCount;             // Number of function parameters
    char paramType[12][32];     // Parameters type (max: 12 parameters)
    char paramName[12][32];     // Parameters name (max: 12 parameters)
    char desc[128];             // Function description (comment)
} FunctionInfo;

char *LoadFileText(const char *fileName, int *length);
void GetDataTypeAndName(const char *typeName, int typeNameLen, char *type, char *name);
void MemoryCopy(void *dest, const void *src, unsigned int count);
int IsTextEqual(const char *text1, const char *text2, unsigned int count);
void CharacterReplace(char *str, char search, char replace, int count);

int main()
{
    int length = 0;
    char *buffer = LoadFileText("raylib.h", &length);

    char **funcLines = NULL;
    funcLines = (char **)malloc(MAX_FUNC_LINES*sizeof(char *));
    for (int i = 0; i < MAX_LINE_LENGTH; i++) funcLines[i] = (char *)calloc(MAX_LINE_LENGTH, sizeof(char));
    
    int funcStart = 0;
    int counter = 0;
    
    for (int i = 2; i < length - 5; i++)
    {
        if ((buffer[i - 1] == '\n') && IsTextEqual(buffer + i, FUNCTION_LINE_DECLARATION, 5))
        {
            funcStart = i;      // Starts a raylib function (RLAPI)
        }
        
        if (buffer[i] == '\n')  // End of line
        {
            if (funcStart != 0)
            {
                MemoryCopy(funcLines[counter], buffer + funcStart, i - funcStart);
                counter++;
                funcStart = 0;
            }
        }
    }
    
    printf("{\n");
    printf("  \"functions_count\": %i,\n", counter);
    printf("  \"functions\": [\n");
    
    free(buffer);       // Unload text buffer
    
    // At this point we have all raylib functions in separate lines
    
    // Functions info data
    FunctionInfo *funcs = (FunctionInfo *)calloc(MAX_FUNC_LINES, sizeof(FunctionInfo));
    
    for (int i = 0; i < counter; i++)
    {
        int funcParamsStart = 0;
        int funcEnd = 0;
        
        // Get return type and function name from func line
        for (int c = 0; (c < MAX_LINE_LENGTH) && (funcLines[i][c] != '\n'); c++)
        {
            if (funcLines[i][c] == '(')     // Starts function parameters
            {
                funcParamsStart = c + 1;
                
                // At this point we have function return type and function name
                char funcRetTypeName[128] = { 0 };
                int funcRetTypeNameLen = c - 6;     // Substract "RLAPI "
                MemoryCopy(funcRetTypeName, &funcLines[i][6], funcRetTypeNameLen);
                
                GetDataTypeAndName(funcRetTypeName, funcRetTypeNameLen, funcs[i].retType, funcs[i].name);
                break;
            }
        }
        
        // Get parameters from func line
        for (int c = funcParamsStart; c < MAX_LINE_LENGTH; c++)
        {
            if (funcLines[i][c] == ',')     // Starts function parameters
            {
                // Get parameter type + name, extract info
                char funcParamTypeName[128] = { 0 };
                int funcParamTypeNameLen = c - funcParamsStart;
                MemoryCopy(funcParamTypeName, &funcLines[i][funcParamsStart], funcParamTypeNameLen);
                
                GetDataTypeAndName(funcParamTypeName, funcParamTypeNameLen, funcs[i].paramType[funcs[i].paramCount], funcs[i].paramName[funcs[i].paramCount]);
                
                funcParamsStart = c + 1;
                if (funcLines[i][c + 1] == ' ') funcParamsStart += 1;
                funcs[i].paramCount++;      // Move to next parameter
            }
            else if (funcLines[i][c] == ')')
            {
                funcEnd = c + 2;
                
                // Check if previous word is void
                if ((funcLines[i][c - 4] == 'v') && (funcLines[i][c - 3] == 'o') && (funcLines[i][c - 2] == 'i') && (funcLines[i][c - 1] == 'd')) break;
                
                // Get parameter type + name, extract info
                char funcParamTypeName[128] = { 0 };
                int funcParamTypeNameLen = c - funcParamsStart;
                MemoryCopy(funcParamTypeName, &funcLines[i][funcParamsStart], funcParamTypeNameLen);
                
                GetDataTypeAndName(funcParamTypeName, funcParamTypeNameLen, funcs[i].paramType[funcs[i].paramCount], funcs[i].paramName[funcs[i].paramCount]);

                funcs[i].paramCount++;      // Move to next parameter
                break;
            }
        }
        
        // Get function description
        for (int c = funcEnd; c < MAX_LINE_LENGTH; c++)
        {
            if (funcLines[i][c] == '/') 
            {
                MemoryCopy(funcs[i].desc, &funcLines[i][c], 127);   // WARNING: Size could be too long for funcLines[i][c]?
                break;
            }
        }
        
        // Print function info
        printf("    {\n");
        printf("      \"name\": \"%s\",\n", funcs[i].name);
        CharacterReplace(funcs[i].desc, '\\', ' ', MAX_LINE_LENGTH);
        CharacterReplace(funcs[i].desc, '"', '\'', MAX_LINE_LENGTH);
        printf("      \"description\": \"%s\",\n", funcs[i].desc + 3);
        //p rintf("      \"index\": %03i\n", i + 1);
        printf("      \"return\": \"%s\"", funcs[i].retType);
        if (funcs[i].paramCount > 0) {
            printf(",\n      \"params\": [\n");
            for (int p = 0; p < funcs[i].paramCount; p++)
            {
                printf("        {\n");
                printf("          \"type\": \"%s\",\n", funcs[i].paramType[p]);
                printf("          \"name\": \"%s\"\n", funcs[i].paramName[p]);
                printf("        }");
                if (p < funcs[i].paramCount - 1) {
                    printf(",\n");
                }
                else {
                    printf("\n");
                }
            }
            printf("      ]\n");
        }
        else {
            printf("\n");
        }
        printf("    }");

        if (i < counter - 1) {
            printf(",\n");
        }
        else {
            printf("\n");
        }
        
        // // Print function info
        // printf("Function %03i:\n", i + 1);
        // printf("    return type: %s\n", funcs[i].retType);
        // printf("           name: %s\n", funcs[i].name);
        // printf("         params: %i\n", funcs[i].paramCount);
        // for (int p = 0; p < funcs[i].paramCount; p++)
        // {
        //     printf("    %i type: %s\n", p + 1, funcs[i].paramType[p]);
        //     printf("    %i name: %s\n", p + 1, funcs[i].paramName[p]);
        // }
        // printf("           desc: %s\n", funcs[i].desc);
    }
    printf("  ]\n");
    printf("}\n");

    for (int i = 0; i < MAX_LINE_LENGTH; i++) free(funcLines[i]);
    free(funcLines);
    
    // So, now we have all the functions decomposed into pieces for further analysis
    
    free(funcs);
}

void GetDataTypeAndName(const char *typeName, int typeNameLen, char *type, char *name)
{
    for (int k = typeNameLen; k > 0; k--)
    {
        if (typeName[k] == ' ')
        {
            // Function name starts at this point (and ret type finishes at this point)
            MemoryCopy(type, typeName, k);
            MemoryCopy(name, typeName + k + 1, typeNameLen - k - 1);
            break;
        }
        else if (typeName[k] == '*')
        {
            MemoryCopy(type, typeName, k + 1);
            MemoryCopy(name, typeName + k + 1, typeNameLen - k - 1);
            break;
        }
    }
}

// Load text data from file, returns a '\0' terminated string
// NOTE: text chars array should be freed manually
char *LoadFileText(const char *fileName, int *length)
{
    char *text = NULL;

    if (fileName != NULL)
    {
        FILE *file = fopen(fileName, "rt");

        if (file != NULL)
        {
            // WARNING: When reading a file as 'text' file,
            // text mode causes carriage return-linefeed translation...
            // ...but using fseek() should return correct byte-offset
            fseek(file, 0, SEEK_END);
            int size = ftell(file);
            fseek(file, 0, SEEK_SET);

            if (size > 0)
            {
                *length = size;
                
                text = (char *)calloc((size + 1), sizeof(char));
                unsigned int count = (unsigned int)fread(text, sizeof(char), size, file);

                // WARNING: \r\n is converted to \n on reading, so,
                // read bytes count gets reduced by the number of lines
                if (count < (unsigned int)size) text = realloc(text, count + 1);

                // Zero-terminate the string
                text[count] = '\0';
            }

            fclose(file);
        }
    }

    return text;
}

// Custom memcpy() to avoid <string.h>
void MemoryCopy(void *dest, const void *src, unsigned int count)
{
    char *srcPtr = (char *)src;
    char *destPtr = (char *)dest;

    for (unsigned int i = 0; i < count; i++) destPtr[i] = srcPtr[i];
}


int IsTextEqual(const char *text1, const char *text2, unsigned int count)
{
    int result = 1;
    
    for (unsigned int i = 0; i < count; i++) 
    {
        if (text1[i] != text2[i]) 
        {
            result = 0;
            break;
        }
    }

    return result;
}

void CharacterReplace(char *str, char search, char replace, int count)
{
    for (int i = 0; i < count; i++) {
    	if(str[i] == search) {
            str[i] = replace;
        }
    }
}
#include <iostream>

const int ARRAY_SIZE = 8;

__int16* unpack_bw(__int8* arr) // ������ �� 8 ������
{
    __int16* result = new __int16[ARRAY_SIZE];
    __asm
    {
        mov eax, result;     // � eax ����� ����������
        mov ecx, arr;        // � ecx ����� ��������� ������� (8 ����)
        pxor mm7, mm7;       // � mm7 0
        pcmpgtb mm7, [ecx];  // ���������� � ����� � ������� ����� �� 00h/ ffh � ����������� �� �����

        movq mm0, [ecx];     // �������� � mm0 64 ���� �� arr
        punpcklbw mm0, mm7;  // ��������������� ����� � ����� � 4 ������� ����� � mm0
        movq [eax], mm0;     // ������������ ����� � ���������

        movq mm0, [ecx];     // �������� � mm0 64 ���� �� arr
        punpckhbw mm0, mm7;  // ��������������� ����� � ����� � 4 ������� ����� � mm0
        movq [eax+8], mm0;   // ���������� ��������� 4 ����� � ���������
        emms;
    }
    return result; //���������� ��������� �� 8 ���� 
}

__int16* add_int16(__int16* op1, __int16* op2)
{
    __int16* result = new __int16[ARRAY_SIZE];
    for (int i = 0; i < ARRAY_SIZE / 4; i++)
    {
        __asm
        {
            //���������� ������ � ��������
            mov eax, op1;
            mov ebx, op2;
            mov ecx, result;

            movq mm0, [eax];
            movq mm1, [ebx];
            paddw mm0, mm1;//��� ������������, ����� � ����������� ������� ������� ��� ������������:)

            movq[ecx], mm0;
            emms;
        }
        //�������� ��������� �� 4 ����� ������
        op1 += 4;
        op2 += 4;
        result += 4;
    }
    result -= 4 * (ARRAY_SIZE / 4);   //���������� ��������� �� ������ �������
    return result;
}

__int16* sub_int16(__int16* op1, __int16* op2)
{
    __int16* result = new __int16[ARRAY_SIZE];
    for (int i = 0; i < ARRAY_SIZE / 4; i++)
    {
        __asm
        {
            mov eax, op1;
            mov ebx, op2;
            mov ecx, result;

            movq mm0, [eax];
            movq mm1, [ebx];
            psubw mm0, mm1;

            movq[ecx], mm0;
            emms;
        }
        op1 += 4;
        op2 += 4;
        result += 4;
    }
    result -= 4 * (ARRAY_SIZE / 4);
    return result;
}

__int16* mul_int16(__int16* op1, __int16* op2)
{
    __int16* result = new __int16[ARRAY_SIZE];
    for (int i = 0; i < ARRAY_SIZE / 4; i++)
    {
        __asm
        {
            mov eax, op1;
            mov ebx, op2;
            mov ecx, result;

            movq mm0, [eax];
            movq mm1, [ebx];
            pmullw mm0, mm1;

            movq[ecx], mm0;
            emms;
        }
        op1 += 4;
        op2 += 4;
        result += 4;
    }
    result -= 4 * (ARRAY_SIZE / 4);
    return result;
}
template<typename T>
void print_array(T* arr) {
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        std::cout << arr[i] << '\t';
    }
    std::cout << std::endl;
}


int main()
{
    __int8 A[ARRAY_SIZE], B[ARRAY_SIZE], C[ARRAY_SIZE];
    __int16 D[ARRAY_SIZE];
    for (int i = 0; i < 8; i++)
    {
        A[i] = rand() % 256 - 128;
        B[i] = rand() % 256 - 128;
        C[i] = rand() % 256 - 128;
        D[i] = rand() % 65536 - 32768; 
    }
    __int16* A_16 = unpack_bw(A);
    std::cout << "A:   ";
    print_array(A_16);
    __int16* B_16 = unpack_bw(B);
    std::cout << "B:   ";
    print_array(B_16);
    __int16* C_16 = unpack_bw(C);
    std::cout << "C:   ";
    print_array(C_16);
    std::cout << "D:   ";
    print_array(D);
    // 3)    F[i]=A[i]-B[i]*C[i]+D[i] , i=1...8;
    __int16* F = add_int16(sub_int16(A_16, mul_int16(B_16, C_16)), D);
    __int16* checkF = new __int16[ARRAY_SIZE];
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        checkF[i] = A_16[i] - B_16[i] * C_16[i] + D[i];
    }
    std::cout << "Using SIMD:\n\t";
    print_array(F);
    std::cout << "Real values:\n\t";
    print_array(checkF);
}
#include <windows.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <wchar.h>  // Для работы с широкими строками (wchar_t)
#include <stdio.h>  // Для использования wsprintf

// Структура для параметров второго потока
struct ThreadParams {
    HWND hWnd;
    int centerX;
    int centerY;
    int diagH;
    int diagV;
};

// Глобальные переменные
HANDLE g_hFirstThread = NULL;
HANDLE g_hSecondThread = NULL;
HANDLE g_hTimer = NULL;
const wchar_t* timer = L"MyTimer";
const double PI = 3.14159;
int g_angle = 0;
bool g_isFirstThreadActive = false;

// Прототипы функций
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI FirstThreadProc(LPVOID);
DWORD WINAPI SecondThreadProc(LPVOID);

// Точка входа

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ThreadsApp";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassEx(&wc);

    HWND hWnd = CreateWindow(
        L"ThreadsApp", L"Многопоточное приложение",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

DWORD WINAPI FirstThreadProc(LPVOID lpParam) {
    HWND hWnd = (HWND)lpParam;

    // Инициализация генератора случайных чисел
    srand((unsigned int)time(0));

    while (true) {
        if (WaitForSingleObject(g_hTimer, INFINITE) == WAIT_OBJECT_0) {
            for (int j = 0; j < 5; j++) {
                // Генерация случайных координат для квадрата
                int x = rand() % 800;  // Ширина окна
                int y = rand() % 600;  // Высота окна

                HDC hdc = GetDC(hWnd);

                // Рисуем квадрат 10x10 пикселей
                RECT rect = { x, y, x + 10, y + 10 };
                FillRect(hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH)); // Рисуем квадрат черного цвета

                Sleep(200); // Ждем 0,2 секунды
                //InvalidateRect(hWnd, NULL, TRUE);
            }
        }
    }
    return 0;
}

// Структура для параметров многочлена
struct PolynomialParams {
    HWND hWnd;      // Дескриптор окна
    int degree;     // Степень многочлена
    double* coeffs; // Массив коэффициентов многочлена
};

// Функция для вычисления коэффициентов первой производной многочлена
void ComputeDerivative(double* coeffs, int degree, double* derivativeCoeffs) {
    // Первая производная: для многочлена a0 + a1*x + a2*x^2 + ... + an*x^n
    // коэффициенты производной будут: a1 + 2*a2*x + 3*a3*x^2 + ... + n*an*x^(n-1)
    for (int i = 1; i <= degree; ++i) {
        derivativeCoeffs[i - 1] = coeffs[i] * i;
    }
}

// Процедура второго потока
DWORD WINAPI SecondThreadProc(LPVOID lpParam) {
    // Преобразуем входной параметр в структуру
    PolynomialParams* params = (PolynomialParams*)lpParam;
    HWND hWnd = params->hWnd;
    int degree = params->degree;
    double* coeffs = params->coeffs;

    // Массив для хранения коэффициентов первой производной
    double* derivativeCoeffs = new double[degree];

    // Вычисление коэффициентов первой производной
    ComputeDerivative(coeffs, degree, derivativeCoeffs);

    // Создаем строку для отображения результата
    wchar_t result[256];  // Буфер для текста
    wchar_t temp[50];     // Временный буфер для формата

    // Формирование строки для вывода
    wcscpy_s(result, sizeof(result) / sizeof(wchar_t), L"Производная: ");
    for (int i = 0; i < degree; ++i) {
        // Выводим коэффициенты производной, ограничивая точность
        swprintf(temp, sizeof(temp) / sizeof(wchar_t), L"%.2f", derivativeCoeffs[i]);
        wcscat_s(result, temp);
        if (i < degree - 1) wcscat_s(result, L" ");
    }

    // Выводим результат на экран
    HDC hdc = GetDC(hWnd);
    TextOut(hdc, 10, 10, result, wcslen(result));  // Выводим текст в окно (на позиции 10, 10)
    ReleaseDC(hWnd, hdc);

    // Освобождаем память
    delete[] derivativeCoeffs;
    delete params;

    return 0;
}


// Оконная процедура
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
    {
        // Создание меню
        HMENU hMenu = CreateMenu();

        AppendMenu(hMenu, MF_STRING, 1001, L"Начать Поток 1");
        AppendMenu(hMenu, MF_STRING, 1002, L"Остановить Поток 1");
        AppendMenu(hMenu, MF_STRING, 1003, L"Начать Поток 2");
        AppendMenu(hMenu, MF_STRING, 1004, L"Запустить дочернее приложение");

        SetMenu(hWnd, hMenu);

        // Создание первого потока в приостановленном состоянии
        g_hFirstThread = CreateThread(
            NULL, 0, FirstThreadProc, (LPVOID)hWnd,
            CREATE_SUSPENDED, NULL
        );

        // Создание таймера
        g_hTimer = CreateWaitableTimer(NULL, FALSE, timer);
        if (g_hTimer) {
            LARGE_INTEGER liDueTime;
            liDueTime.QuadPart = -100000000 * 0.5;
            SetWaitableTimer(g_hTimer, &liDueTime, 4000, NULL, NULL, FALSE);
        }
    }
    break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1001: // Запуск первого потока
            if (g_hFirstThread) {
                ResumeThread(g_hFirstThread);
                g_isFirstThreadActive = true;
            }
            break;

        case 1002: // Приостановка первого потока
            if (g_hFirstThread) {
                SuspendThread(g_hFirstThread);
                g_isFirstThreadActive = false;
            }
            break;

        case 1003: // Создание второго потока
        {
            PolynomialParams* params = new PolynomialParams;
            params->hWnd = hWnd;
            params->degree = 3;  // Пример степени многочлена (например, 3)
            params->coeffs = new double[params->degree + 1];  // Массив коэффициентов

            // Заполняем коэффициенты многочлена
            params->coeffs[0] = 2.0;  // a0
            params->coeffs[1] = 5.0;  // a1
            params->coeffs[2] = 3.0;  // a2
            params->coeffs[3] = 1.0;  // a3

            g_hSecondThread = CreateThread(
                NULL, 0, SecondThreadProc, (LPVOID)params,
                0, NULL
            );
        }
        break;


        case 1004: // Запуск дочернего приложения
        {
            STARTUPINFO si = { 0 };
            PROCESS_INFORMATION pi = { 0 };
            si.cb = sizeof(si);

            CreateProcess(
                L"C:\\Users\\xxx\\Desktop\\ChildApp\\x64\\Debug\\ChildApp.exe", NULL, NULL, NULL, FALSE,
                0, NULL, NULL, &si, &pi
            );
        }
        break;
        }
        break;

    case WM_DESTROY:
        if (g_hFirstThread) CloseHandle(g_hFirstThread);
        if (g_hSecondThread) CloseHandle(g_hSecondThread);
        if (g_hTimer) CloseHandle(g_hTimer);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

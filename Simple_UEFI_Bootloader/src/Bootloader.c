#include "Bootloader.h"

STATIC CONST CHAR16 AppleFirmwareVendor[6] = L"Apple";
UINT8 IsApple = 0;

STATIC EFI_STATUS ShowWelcomeAndConfirm(VOID);
EFI_STATUS RunMyNameGame(GPU_CONFIG *Graphics);

//==================================================================================================================================
//  efi_main
//  
//  Función principal que ejecuta el firmware UEFI al arrancar. 
//  Prepara la consola, inicializa los gráficos usando GOP y maneja el flujo principal 
//  entre el menú de bienvenida y el juego.
//==================================================================================================================================

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  InitializeLib(ImageHandle, SystemTable);

  EFI_STATUS Status;

  // Limpiamos la consola de texto para empezar con la pantalla vacía
  Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  if(EFI_ERROR(Status))
  {
    Print(L"NOTE: Could not clear the screen.\r\n");
  }

#ifdef DISABLE_UEFI_WATCHDOG_TIMER
  // Apagamos el timer de UEFI para que la compu no se reinicie sola si pasamos mucho rato jugando
  Status = BS->SetWatchdogTimer(0, 0, 0, NULL);
  if(EFI_ERROR(Status))
  {
    Print(L"Error stopping watchdog timer.\r\n");
  }
#endif

  // Parche para equipos Mac: a veces necesitan que se les fuerce el modo texto para que se vea bien
  if(compare(ST->FirmwareVendor, AppleFirmwareVendor, 6))
  {
    IsApple = 1;

    EFI_CONSOLE_CONTROL_PROTOCOL *ConsoleMode = NULL;
    Status = LibLocateProtocol(&ConsoleControlProtocol, (VOID**)&ConsoleMode);

    if(!EFI_ERROR(Status) && ConsoleMode != NULL)
    {
      EFI_CONSOLE_CONTROL_SCREEN_MODE Current_Mode;
      ConsoleMode->GetMode(ConsoleMode, &Current_Mode, NULL, NULL);

      if(Current_Mode != EfiConsoleControlScreenText)
      {
        ConsoleMode->SetMode(ConsoleMode, EfiConsoleControlScreenText);
      }
    }
  }

  // Reservamos memoria para la estructura que va a manejar los gráficos del juego
  GPU_CONFIG *Graphics;
  Status = ST->BootServices->AllocatePool(EfiLoaderData, sizeof(GPU_CONFIG), (void**)&Graphics);
  if(EFI_ERROR(Status))
  {
    Print(L"Graphics AllocatePool error. 0x%llx\r\n", Status);
    return Status;
  }

  // Inicializamos el protocolo GOP (Graphics Output Protocol)
  Status = InitUEFI_GOP(ImageHandle, Graphics);
  if(EFI_ERROR(Status))
  {
    Print(L"InitUEFI_GOP error. 0x%llx\r\n", Status);
    Keywait(L"\r\n");
    return Status;
  }

  // Volvemos a limpiar la pantalla antes de tirar el menú principal
  ST->ConOut->ClearScreen(ST->ConOut);

  // Mostramos el menú y esperamos a que el usuario decida si jugar o salir
  Status = ShowWelcomeAndConfirm();
  if(EFI_ERROR(Status))
  {
    Print(L"Input error. 0x%llx\r\n", Status);
    return Status;
  }

  // Si el usuario presionó ESC, cancelamos todo y salimos
  if(Status == EFI_ABORTED)
  {
    ST->ConOut->ClearScreen(ST->ConOut);
    Print(L"My Name cancelled by user.\r\n");
    return EFI_SUCCESS;
  }

  // Si dio ENTER, llamamos a la función que arranca el juego en sí
  Status = RunMyNameGame(Graphics);

  // Cuando el juego termina, limpiamos la pantalla y mostramos el mensaje de salida
  ST->ConOut->ClearScreen(ST->ConOut);

  if(EFI_ERROR(Status))
  {
    Print(L"My Name ended with error. 0x%llx\r\n", Status);
  }
  else
  {
    Print(L"My Name finished.\r\n");
  }

  Keywait(L"\r\n");
  return Status;
}

//==================================================================================================================================
//  ShowWelcomeAndConfirm
//
//  Muestra el título, las instrucciones y los controles en la pantalla.
//  Luego se queda en un ciclo esperando a que se presione ENTER para iniciar
//  o ESC para abortar.
//==================================================================================================================================

STATIC EFI_STATUS ShowWelcomeAndConfirm(VOID)
{
  EFI_STATUS Status;
  EFI_INPUT_KEY Key;

  Print(L"============================================================\r\n");
  Print(L"                        MY NAME\r\n");
  Print(L"============================================================\r\n\r\n");

  Print(L"Welcome to the My Name.\r\n\r\n");
  Print(L"This program will display the names of both team members\r\n");
  Print(L"in a random screen position and allow transformations with\r\n");
  Print(L"the arrow keys.\r\n\r\n");

  Print(L"Controls planned for the game:\r\n");
  Print(L"  LEFT  ARROW  - rotate left\r\n");
  Print(L"  RIGHT ARROW  - rotate right\r\n");
  Print(L"  UP    ARROW  - upper transformation\r\n");
  Print(L"  DOWN  ARROW  - lower transformation\r\n");
  Print(L"  R            - restart\r\n");
  Print(L"  ESC          - exit\r\n\r\n");

  Print(L"Press ENTER to start or ESC to cancel.\r\n");

  // Limpiamos la entrada del teclado por si había alguna tecla pegada
  Status = ST->ConIn->Reset(ST->ConIn, FALSE);
  if(EFI_ERROR(Status))
  {
    return Status;
  }

  // Nos quedamos atrapados aquí leyendo el teclado hasta que toquen ENTER o ESC
  while(1)
  {
    while((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);

    if(EFI_ERROR(Status))
    {
      return Status;
    }

    // Si toca ENTER, devolvemos SUCCESS para que el main arranque el juego
    if(Key.UnicodeChar == CHAR_CARRIAGE_RETURN)
    {
      return EFI_SUCCESS;
    }

    // Si toca ESC, devolvemos ABORTED para que el main cierre el programa
    if(Key.ScanCode == SCAN_ESC)
    {
      return EFI_ABORTED;
    }
  }
}

//==================================================================================================================================
//  Keywait
//  
//  Función de ayuda para pausar el programa. Imprime un string personalizado,
//  avisa que presiones una tecla, y se queda esperando hasta que lo hagas.
//==================================================================================================================================

EFI_STATUS Keywait(CHAR16 *String)
{
  EFI_STATUS Status;
  EFI_INPUT_KEY Key;

  Print(String);

  Status = ST->ConOut->OutputString(ST->ConOut, L"Press any key to continue...");
  if(EFI_ERROR(Status))
  {
    return Status;
  }

  Status = ST->ConIn->Reset(ST->ConIn, FALSE);
  if(EFI_ERROR(Status))
  {
    return Status;
  }

  // Espera a que se presione cualquier tecla
  while((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);

  Status = ST->ConIn->Reset(ST->ConIn, FALSE);
  if(EFI_ERROR(Status))
  {
    return Status;
  }

  Print(L"\r\n");
  return Status;
}
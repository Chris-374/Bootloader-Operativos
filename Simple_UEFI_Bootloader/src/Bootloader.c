//==================================================================================================================================
//  Simple UEFI Bootloader: Bootloader Entrypoint
//==================================================================================================================================

#include "Bootloader.h"

STATIC CONST CHAR16 AppleFirmwareVendor[6] = L"Apple";
UINT8 IsApple = 0;

STATIC EFI_STATUS ShowWelcomeAndConfirm(VOID);
EFI_STATUS RunMyNameGame(GPU_CONFIG *Graphics);

//==================================================================================================================================
//  efi_main: Main Function
//==================================================================================================================================

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  InitializeLib(ImageHandle, SystemTable);

  EFI_STATUS Status;

  // Clear text console
  Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  if(EFI_ERROR(Status))
  {
    Print(L"NOTE: Could not clear the screen.\r\n");
  }

#ifdef DISABLE_UEFI_WATCHDOG_TIMER
  Status = BS->SetWatchdogTimer(0, 0, 0, NULL);
  if(EFI_ERROR(Status))
  {
    Print(L"Error stopping watchdog timer.\r\n");
  }
#endif

  // Macs sometimes need explicit switch to text mode
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

  // Allocate graphics config
  GPU_CONFIG *Graphics;
  Status = ST->BootServices->AllocatePool(EfiLoaderData, sizeof(GPU_CONFIG), (void**)&Graphics);
  if(EFI_ERROR(Status))
  {
    Print(L"Graphics AllocatePool error. 0x%llx\r\n", Status);
    return Status;
  }

  // Init graphics
  Status = InitUEFI_GOP(ImageHandle, Graphics);
  if(EFI_ERROR(Status))
  {
    Print(L"InitUEFI_GOP error. 0x%llx\r\n", Status);
    Keywait(L"\r\n");
    return Status;
  }

  // Back to text screen for welcome / confirmation
  ST->ConOut->ClearScreen(ST->ConOut);

  // Welcome + confirmation
  Status = ShowWelcomeAndConfirm();
  if(EFI_ERROR(Status))
  {
    Print(L"Input error. 0x%llx\r\n", Status);
    return Status;
  }

  if(Status == EFI_ABORTED)
  {
    ST->ConOut->ClearScreen(ST->ConOut);
    Print(L"My Name cancelled by user.\r\n");
    return EFI_SUCCESS;
  }

  // Run the game
  Status = RunMyNameGame(Graphics);

  // End message if the game returns
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
//==================================================================================================================================
//
// ENTER = start
// ESC   = exit
//

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

  Status = ST->ConIn->Reset(ST->ConIn, FALSE);
  if(EFI_ERROR(Status))
  {
    return Status;
  }

  while(1)
  {
    while((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);

    if(EFI_ERROR(Status))
    {
      return Status;
    }

    // ENTER
    if(Key.UnicodeChar == CHAR_CARRIAGE_RETURN)
    {
      return EFI_SUCCESS;
    }

    // ESC
    if(Key.ScanCode == SCAN_ESC)
    {
      return EFI_ABORTED;
    }
  }
}

//==================================================================================================================================
//  Keywait: Pause
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

  while((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);

  Status = ST->ConIn->Reset(ST->ConIn, FALSE);
  if(EFI_ERROR(Status))
  {
    return Status;
  }

  Print(L"\r\n");
  return Status;
}
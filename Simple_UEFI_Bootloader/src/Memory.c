#include "Bootloader.h"

//
//  compare
//
//  Esta función nos sirve para comparar dos bloques de memoria byte por byte.
//  Si los dos bloques son exactamente iguales, devuelve 1. Si encuentra alguna diferencia,
//  devuelve 0 de inmediato. La usamos principalmente para verificar si estamos corriendo en un 
//  firmware de Apple comparando el vendor string.
//

// Variable 'comparelength' is in bytes
UINT8 compare(const void* firstitem, const void* seconditem, UINT64 comparelength)
{
  // Using const since this is a read-only operation: absolutely nothing should be changed here.
  const UINT8 *one = firstitem, *two = seconditem;
  for (UINT64 i = 0; i < comparelength; i++)
  {
    if(one[i] != two[i])
    {
      return 0;
    }
  }
  return 1;
}


//
//  VerifyZeroMem
//
//  Revisa un segmento de memoria para asegurarse de que esté completamente vacío (lleno de ceros).
//  Devuelve 0 si toda la memoria está limpia, o 1 si encuentra algún dato atravesado.
//  Es útil para validar que la memoria que pedimos realmente está limpia antes de meterle datos.
//

UINT8 VerifyZeroMem(UINT64 NumBytes, UINT64 BaseAddr) // BaseAddr is a 64-bit unsigned int whose value is the memory address
{
  for(UINT64 i = 0; i < NumBytes; i++)
  {
    if(*(((UINT8*)BaseAddr) + i) != 0)
    {
      return 1;
    }
  }
  return 0;
}

//
//  ActuallyFreeAddress
//
//  Si UEFI se pone necio y no nos quiere dar memoria con los métodos normales (AllocateAnyPages),
//  esta función busca "a la fuerza" el siguiente bloque libre (EfiConventionalMemory)
//  en el mapa de memoria del sistema que sea mayor a la dirección que le pasamos por parámetro (OldAddress).
//

EFI_PHYSICAL_ADDRESS ActuallyFreeAddress(UINT64 pages, EFI_PHYSICAL_ADDRESS OldAddress)
{
  EFI_STATUS memmap_status;
  UINTN MemMapSize = 0, MemMapKey, MemMapDescriptorSize;
  UINT32 MemMapDescriptorVersion;
  EFI_MEMORY_DESCRIPTOR * MemMap = NULL;
  EFI_MEMORY_DESCRIPTOR * Piece;

  memmap_status = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
  if(memmap_status == EFI_BUFFER_TOO_SMALL)
  {
    MemMapSize += MemMapDescriptorSize;
    memmap_status = BS->AllocatePool(EfiBootServicesData, MemMapSize, (void **)&MemMap); // Allocate pool for MemMap
    if(EFI_ERROR(memmap_status)) // Error! Wouldn't be safe to continue.
    {
      Print(L"ActuallyFreeAddress MemMap AllocatePool error. 0x%llx\r\n", memmap_status);
      return ~0ULL;
    }
    memmap_status = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
  }
  if(EFI_ERROR(memmap_status))
  {
    Print(L"Error getting memory map for ActuallyFreeAddress. 0x%llx\r\n", memmap_status);
    return ~0ULL;
  }

  // Multiply NumberOfPages by EFI_PAGE_SIZE to get the end address... which should just be the start of the next section.
  // Check for EfiConventionalMemory in the map
  for(Piece = MemMap; Piece < (EFI_MEMORY_DESCRIPTOR*)((UINT8*)MemMap + MemMapSize); Piece = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)Piece + MemMapDescriptorSize))
  {
    // Within each compatible EfiConventionalMemory, look for space
    if((Piece->Type == EfiConventionalMemory) && (Piece->NumberOfPages >= pages) && (Piece->PhysicalStart > OldAddress))
    {
      break;
    }
  }

  // Loop ended without a DiscoveredAddress
  if(Piece >= (EFI_MEMORY_DESCRIPTOR*)((UINT8*)MemMap + MemMapSize))
  {
    // Return address -1, which will cause AllocatePages to fail
#ifdef MEMORY_CHECK_INFO
    Print(L"No more free addresses...\r\n");
#endif
    return ~0ULL;
  }

  memmap_status = BS->FreePool(MemMap);
  if(EFI_ERROR(memmap_status))
  {
    Print(L"Error freeing ActuallyFreeAddress memmap pool. 0x%llx\r\n", memmap_status);
  }

  return Piece->PhysicalStart;
}

//
//  ActuallyFreeAddressByPage
//
//  Hace lo mismo que ActuallyFreeAddress pero es más intensa. 
//  En vez de brincar por bloques enteros de memoria, busca página por página dentro 
//  de los rangos válidos hasta encontrar espacio libre. Es un plan C por si 
//  el mapa de memoria de la BIOS está muy fragmentado o tiene bugs raros.
//

EFI_PHYSICAL_ADDRESS ActuallyFreeAddressByPage(UINT64 pages, EFI_PHYSICAL_ADDRESS OldAddress)
{
  EFI_STATUS memmap_status;
  UINTN MemMapSize = 0, MemMapKey, MemMapDescriptorSize;
  UINT32 MemMapDescriptorVersion;
  EFI_MEMORY_DESCRIPTOR * MemMap = NULL;
  EFI_MEMORY_DESCRIPTOR * Piece;
  EFI_PHYSICAL_ADDRESS PhysicalEnd;
  EFI_PHYSICAL_ADDRESS DiscoveredAddress;

  memmap_status = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
  if(memmap_status == EFI_BUFFER_TOO_SMALL)
  {
    MemMapSize += MemMapDescriptorSize;
    memmap_status = BS->AllocatePool(EfiBootServicesData, MemMapSize, (void **)&MemMap); // Allocate pool for MemMap
    if(EFI_ERROR(memmap_status)) // Error! Wouldn't be safe to continue.
    {
      Print(L"ActuallyFreeAddressByPage MemMap AllocatePool error. 0x%llx\r\n", memmap_status);
      return ~0ULL;
    }
    memmap_status = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
  }
  if(EFI_ERROR(memmap_status))
  {
    Print(L"Error getting memory map for ActuallyFreeAddressByPage. 0x%llx\r\n", memmap_status);
    return ~0ULL;
  }

  // Multiply NumberOfPages by EFI_PAGE_SIZE to get the end address... which should just be the start of the next section.
  // Check for EfiConventionalMemory in the map
  for(Piece = MemMap; Piece < (EFI_MEMORY_DESCRIPTOR*)((UINT8*)MemMap + MemMapSize); Piece = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)Piece + MemMapDescriptorSize))
  {
    // Within each compatible EfiConventionalMemory, look for space
    if((Piece->Type == EfiConventionalMemory) && (Piece->NumberOfPages >= pages))
    {
      PhysicalEnd = Piece->PhysicalStart + (Piece->NumberOfPages << EFI_PAGE_SHIFT) - EFI_PAGE_MASK; // Get the end of this range, and use it to set a bound on the range (define a max returnable address).
      // (pages*EFI_PAGE_SIZE) or (pages << EFI_PAGE_SHIFT) gives the size the kernel would take up in memory
      if((OldAddress >= Piece->PhysicalStart) && ((OldAddress + (pages << EFI_PAGE_SHIFT)) < PhysicalEnd)) // Bounds check on OldAddress
      {
        // Return the next available page's address in the range. We need to go page-by-page for the really buggy systems.
        DiscoveredAddress = OldAddress + EFI_PAGE_SIZE; // Left shift EFI_PAGE_SIZE by 1 or 2 to check every 0x10 or 0x100 pages (must also modify the above PhysicalEnd bound check)
        break;
        // If PhysicalEnd == OldAddress, we need to go to the next EfiConventionalMemory range
      }
      else if(Piece->PhysicalStart > OldAddress) // Try a new range
      {
        DiscoveredAddress = Piece->PhysicalStart;
        break;
      }
    }
  }

  // Loop ended without a DiscoveredAddress
  if(Piece >= (EFI_MEMORY_DESCRIPTOR*)((UINT8*)MemMap + MemMapSize))
  {
    // Return address -1, which will cause AllocatePages to fail
#ifdef MEMORY_CHECK_INFO
    Print(L"No more free addresses by page...\r\n");
#endif
    return ~0ULL;
  }

  memmap_status = BS->FreePool(MemMap);
  if(EFI_ERROR(memmap_status))
  {
    Print(L"Error freeing ActuallyFreeAddressByPage memmap pool. 0x%llx\r\n", memmap_status);
  }

  return DiscoveredAddress;
}

//
//  print_memmap
//
//  Esta es solo una herramienta de debug que hicimos. Pide todo el mapa de memoria
//  a la BIOS y lo imprime bonito en pantalla, diciendo qué tipo de memoria hay
//  en cada dirección y cuántas páginas ocupa. Sirve para ver qué está pasando 
//  si se nos está acabando el espacio.
//

// This array is a global variable so that it can be made static, which helps prevent a stack overflow if it ever needs to lengthen.
STATIC CONST CHAR16 mem_types[16][27] = {
      L"EfiReservedMemoryType     ",
      L"EfiLoaderCode             ",
      L"EfiLoaderData             ",
      L"EfiBootServicesCode       ",
      L"EfiBootServicesData       ",
      L"EfiRuntimeServicesCode    ",
      L"EfiRuntimeServicesData    ",
      L"EfiConventionalMemory     ",
      L"EfiUnusableMemory         ",
      L"EfiACPIReclaimMemory      ",
      L"EfiACPIMemoryNVS          ",
      L"EfiMemoryMappedIO         ",
      L"EfiMemoryMappedIOPortSpace",
      L"EfiPalCode                ",
      L"EfiPersistentMemory       ",
      L"EfiMaxMemoryType          "
};

VOID print_memmap()
{
  EFI_STATUS memmap_status;
  UINTN MemMapSize = 0, MemMapKey, MemMapDescriptorSize;
  UINT32 MemMapDescriptorVersion;
  EFI_MEMORY_DESCRIPTOR * MemMap = NULL;
  EFI_MEMORY_DESCRIPTOR * Piece;
  UINT16 line = 0;

  memmap_status = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
  if(memmap_status == EFI_BUFFER_TOO_SMALL)
  {
    MemMapSize += MemMapDescriptorSize;
    memmap_status = BS->AllocatePool(EfiBootServicesData, MemMapSize, (void **)&MemMap); // Allocate pool for MemMap
    if(EFI_ERROR(memmap_status)) // Error! Wouldn't be safe to continue.
    {
      Print(L"MemMap AllocatePool error. 0x%llx\r\n", memmap_status);
      return;
    }
    memmap_status = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
  }
  if(EFI_ERROR(memmap_status))
  {
    Print(L"Error getting memory map for printing. 0x%llx\r\n", memmap_status);
  }

  Print(L"MemMapSize: %llu, MemMapDescriptorSize: %llu, MemMapDescriptorVersion: 0x%x\r\n", MemMapSize, MemMapDescriptorSize, MemMapDescriptorVersion);

  // There's no virtual addressing yet, so there's no need to see Piece->VirtualStart
  // Multiply NumOfPages by EFI_PAGE_SIZE or do (NumOfPages << EFI_PAGE_SHIFT) to get the end address... which should just be the start of the next section.
  for(Piece = MemMap; Piece < (EFI_MEMORY_DESCRIPTOR*)((UINT8*)MemMap + MemMapSize); Piece = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)Piece + MemMapDescriptorSize))
  {
    if(line%20 == 0)
    {
      Keywait(L"\0");
      Print(L"#   Memory Type                Phys Addr Start   Num Of Pages   Attr\r\n");
    }

    Print(L"%2hu: %s 0x%016llx 0x%llx 0x%llx\r\n", line, mem_types[Piece->Type], Piece->PhysicalStart, Piece->NumberOfPages, Piece->Attribute);
    line++;
  }

  memmap_status = BS->FreePool(MemMap);
  if(EFI_ERROR(memmap_status))
  {
    Print(L"Error freeing print_memmap pool. 0x%llx\r\n", memmap_status);
  }
}
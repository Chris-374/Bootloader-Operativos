#include "Bootloader.h"

//
// Nombres de los integrantes del grupo que se van a dibujar en pantalla
//
STATIC CONST CHAR8 NAME_1[] = "CHRISTIAN";
STATIC CONST CHAR8 NAME_2[] = "JORGE";

// Configuraciones de color
#define BG_R  10
#define BG_G  10
#define BG_B  30

#define FG_R  255
#define FG_G  255
#define FG_B  255

#define INFO_R  120
#define INFO_G  200
#define INFO_B  255

// Ajustes visuales para el texto
#define LETTER_SCALE   6
#define LETTER_SPACING 2
#define WORD_SPACING   8
#define NAME_GAP       18
#define SCREEN_MARGIN  16

// Enumeración de las transformaciones que puede sufrir el texto
typedef enum
{
  TRANSFORM_NORMAL = 0,
  TRANSFORM_ROT_LEFT_90,
  TRANSFORM_ROT_RIGHT_90,
  TRANSFORM_FLIP_VERTICAL,
  TRANSFORM_ROT_180
} MYNAME_TRANSFORM;

// Estructura que guarda toda la info del framebuffer (resolución, formato de pixeles, etc.)
typedef struct
{
  UINT32 Width;
  UINT32 Height;
  UINT32 PixelsPerScanLine;
  EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
  EFI_PIXEL_BITMASK PixelInfo;
  UINT32 *FrameBuffer;
} DRAW_CONTEXT;

// Estructura para guardar el ancho y alto calculado de un texto
typedef struct
{
  UINTN Width;
  UINTN Height;
} TEXT_EXTENTS;

// Definición de un caracter en nuestra fuente custom de 5x7 pixeles
typedef struct
{
  CHAR8 c;
  UINT8 rows[7];
} FONT5X7;

STATIC EFI_STATUS WaitKey(EFI_INPUT_KEY *Key);
STATIC VOID ClearScreenColor(const DRAW_CONTEXT *Ctx, UINT8 R, UINT8 G, UINT8 B);
STATIC VOID PutPixel(const DRAW_CONTEXT *Ctx, INTN X, INTN Y, UINT8 R, UINT8 G, UINT8 B);
STATIC VOID FillRect(const DRAW_CONTEXT *Ctx, INTN X, INTN Y, UINTN W, UINTN H, UINT8 R, UINT8 G, UINT8 B);
STATIC VOID DrawGlyph5x7(const DRAW_CONTEXT *Ctx, CHAR8 Ch, INTN X, INTN Y, UINTN Scale, MYNAME_TRANSFORM Transform, UINT8 R, UINT8 G, UINT8 B);
STATIC VOID DrawText5x7(const DRAW_CONTEXT *Ctx, const CHAR8 *Text, INTN X, INTN Y, UINTN Scale, MYNAME_TRANSFORM Transform, UINT8 R, UINT8 G, UINT8 B);
STATIC TEXT_EXTENTS MeasureText5x7(const CHAR8 *Text, UINTN Scale, MYNAME_TRANSFORM Transform);
STATIC UINT32 PackColor(const DRAW_CONTEXT *Ctx, UINT8 R, UINT8 G, UINT8 B);
STATIC UINTN AsciiLen(const CHAR8 *Str);
STATIC const FONT5X7 *FindGlyph(CHAR8 Ch);
STATIC VOID GetRandomPosition(const DRAW_CONTEXT *Ctx, const CHAR8 *Name1, const CHAR8 *Name2, UINTN Scale, MYNAME_TRANSFORM Transform, INTN *OutX, INTN *OutY);
STATIC VOID DrawScene(const DRAW_CONTEXT *Ctx, const CHAR8 *Name1, const CHAR8 *Name2, INTN X, INTN Y, MYNAME_TRANSFORM Transform);
STATIC VOID DrawInstructionBar(const DRAW_CONTEXT *Ctx);
STATIC VOID DrawCenteredBox(const DRAW_CONTEXT *Ctx, UINTN BoxW, UINTN BoxH, UINT8 R, UINT8 G, UINT8 B);

//
// Fuente 5x7 básica para letras mayúsculas, números y algunos símbolos.
// Cada valor hex representa una fila de 5 bits, indicando qué pixeles se pintan.
//
STATIC CONST FONT5X7 FontTable[] =
{
  {'A',{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},
  {'B',{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
  {'C',{0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
  {'D',{0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}},
  {'E',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
  {'F',{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
  {'G',{0x0E,0x11,0x10,0x10,0x13,0x11,0x0E}},
  {'H',{0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
  {'I',{0x1F,0x04,0x04,0x04,0x04,0x04,0x1F}},
  {'J',{0x01,0x01,0x01,0x01,0x11,0x11,0x0E}},
  {'K',{0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
  {'L',{0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
  {'M',{0x11,0x1B,0x15,0x15,0x11,0x11,0x11}},
  {'N',{0x11,0x11,0x19,0x15,0x13,0x11,0x11}},
  {'O',{0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
  {'P',{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
  {'Q',{0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}},
  {'R',{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
  {'S',{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
  {'T',{0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
  {'U',{0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
  {'V',{0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
  {'W',{0x11,0x11,0x11,0x15,0x15,0x15,0x0A}},
  {'X',{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}},
  {'Y',{0x11,0x11,0x0A,0x04,0x04,0x04,0x04}},
  {'Z',{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},

  {'0',{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},
  {'1',{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
  {'2',{0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
  {'3',{0x1E,0x01,0x01,0x06,0x01,0x01,0x1E}},
  {'4',{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},
  {'5',{0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}},
  {'6',{0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E}},
  {'7',{0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},
  {'8',{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},
  {'9',{0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}},

  {'-',{0x00,0x00,0x00,0x1F,0x00,0x00,0x00}},
  {' ',{0x00,0x00,0x00,0x00,0x00,0x00,0x00}},

  {'\0',{0,0,0,0,0,0,0}}
};

//
//  RunMyNameGame
//
//  El corazón del juego. Extrae la información del GOP (Graphics Output Protocol) 
//  para saber de qué tamaño es la pantalla y cómo dibujar. Luego tira los nombres 
//  en una posición random inicial y entra en un ciclo infinito esperando las 
//  teclas para aplicar las transformaciones (rotaciones).
//

EFI_STATUS RunMyNameGame(GPU_CONFIG *Graphics)
{
  if(Graphics == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  if(Graphics->NumberOfFrameBuffers == 0)
  {
    return EFI_NOT_FOUND;
  }

  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode = &Graphics->GPUArray[0];
  if(Mode->Info == NULL || Mode->FrameBufferBase == 0)
  {
    return EFI_NOT_READY;
  }

  DRAW_CONTEXT Ctx;
  Ctx.Width = Mode->Info->HorizontalResolution;
  Ctx.Height = Mode->Info->VerticalResolution;
  Ctx.PixelsPerScanLine = Mode->Info->PixelsPerScanLine;
  Ctx.PixelFormat = Mode->Info->PixelFormat;
  Ctx.PixelInfo = Mode->Info->PixelInformation;
  Ctx.FrameBuffer = (UINT32*)(UINTN)Mode->FrameBufferBase;

  MYNAME_TRANSFORM Transform = TRANSFORM_NORMAL;
  INTN PosX = 0;
  INTN PosY = 0;

  GetRandomPosition(&Ctx, NAME_1, NAME_2, LETTER_SCALE, Transform, &PosX, &PosY);
  DrawScene(&Ctx, NAME_1, NAME_2, PosX, PosY, Transform);

  while(1)
  {
    EFI_INPUT_KEY Key;
    EFI_STATUS Status = WaitKey(&Key);
    if(EFI_ERROR(Status))
    {
      return Status;
    }

    if(Key.ScanCode == SCAN_ESC)
    {
      return EFI_SUCCESS;
    }

    if(Key.UnicodeChar == L'r' || Key.UnicodeChar == L'R')
    {
      Transform = TRANSFORM_NORMAL;
      GetRandomPosition(&Ctx, NAME_1, NAME_2, LETTER_SCALE, Transform, &PosX, &PosY);
      DrawScene(&Ctx, NAME_1, NAME_2, PosX, PosY, Transform);
      continue;
    }

    if(Key.ScanCode == SCAN_LEFT)
    {
      Transform = TRANSFORM_ROT_LEFT_90;
      DrawScene(&Ctx, NAME_1, NAME_2, PosX, PosY, Transform);
      continue;
    }

    if(Key.ScanCode == SCAN_RIGHT)
    {
      Transform = TRANSFORM_ROT_RIGHT_90;
      DrawScene(&Ctx, NAME_1, NAME_2, PosX, PosY, Transform);
      continue;
    }

    if(Key.ScanCode == SCAN_UP)
    {
      Transform = TRANSFORM_FLIP_VERTICAL;
      DrawScene(&Ctx, NAME_1, NAME_2, PosX, PosY, Transform);
      continue;
    }

    if(Key.ScanCode == SCAN_DOWN)
    {
      Transform = TRANSFORM_ROT_180;
      DrawScene(&Ctx, NAME_1, NAME_2, PosX, PosY, Transform);
      continue;
    }
  }
}

//
//  WaitKey
//
//  Se queda pegada esperando que el usuario presione una tecla. 
//  Limpia el buffer antes para no comerse teclas fantasma.
//

STATIC EFI_STATUS WaitKey(EFI_INPUT_KEY *Key)
{
  EFI_STATUS Status;

  if(Key == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = ST->ConIn->Reset(ST->ConIn, FALSE);
  if(EFI_ERROR(Status))
  {
    return Status;
  }

  while((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, Key)) == EFI_NOT_READY);

  return Status;
}

//
//  DrawScene
//
//  Arma la pantalla completa. Primero limpia el fondo, luego pinta 
//  una caja en el centro (solo de adorno), y encima de eso dibuja 
//  los dos nombres, aplicando la transformación de matriz que corresponda 
//  y asegurándose de que no se salgan de la pantalla por accidente.
//

STATIC VOID DrawScene(const DRAW_CONTEXT *Ctx, const CHAR8 *Name1, const CHAR8 *Name2, INTN X, INTN Y, MYNAME_TRANSFORM Transform)
{
    ClearScreenColor(Ctx, BG_R, BG_G, BG_B);

    TEXT_EXTENTS T1 = MeasureText5x7(Name1, LETTER_SCALE, Transform);
    TEXT_EXTENTS T2 = MeasureText5x7(Name2, LETTER_SCALE, Transform);

    UINTN TotalW = (T1.Width > T2.Width ? T1.Width : T2.Width);
    UINTN TotalH = T1.Height + NAME_GAP + T2.Height;

    UINTN BoxW = TotalW + 40;
    UINTN BoxH = TotalH + 40;

    INTN BoxX = ((INTN)Ctx->Width - (INTN)BoxW) / 2;
    INTN BoxY = ((INTN)Ctx->Height - (INTN)BoxH) / 2;

    FillRect(Ctx, BoxX, BoxY, BoxW, BoxH, 40, 40, 70);
    FillRect(Ctx, BoxX + 6, BoxY + 6, BoxW - 12, BoxH - 12, BG_R, BG_G, BG_B);

    INTN StartX = X - ((INTN)TotalW / 2);
    INTN StartY = Y - ((INTN)TotalH / 2);

    if(StartX < SCREEN_MARGIN) StartX = SCREEN_MARGIN;
    if(StartY < SCREEN_MARGIN) StartY = SCREEN_MARGIN;

    if(StartX + (INTN)TotalW >= (INTN)Ctx->Width - SCREEN_MARGIN)
        StartX = (INTN)Ctx->Width - SCREEN_MARGIN - (INTN)TotalW;

    if(StartY + (INTN)TotalH >= (INTN)Ctx->Height - SCREEN_MARGIN - 50)
        StartY = (INTN)Ctx->Height - SCREEN_MARGIN - 50 - (INTN)TotalH;

    DrawText5x7(Ctx, Name1, StartX, StartY, LETTER_SCALE, Transform, FG_R, FG_G, FG_B);
    DrawText5x7(Ctx, Name2, StartX, StartY + (INTN)T1.Height + NAME_GAP, LETTER_SCALE, Transform, FG_R, FG_G, FG_B);

    DrawInstructionBar(Ctx);
}

//
//  DrawInstructionBar
//
//  Pinta la barrita negra abajo con las letras azules que te recuerda 
//  qué hace cada flecha, la R y el ESC, para que uno no se pierda.
//

STATIC VOID DrawInstructionBar(const DRAW_CONTEXT *Ctx)
{
  FillRect(Ctx, 0, (INTN)Ctx->Height - 42, Ctx->Width, 42, 0, 0, 0);

  DrawText5x7(Ctx, "LEFT RIGHT UP DOWN", 14, (INTN)Ctx->Height - 34, 2, TRANSFORM_NORMAL, INFO_R, INFO_G, INFO_B);
  DrawText5x7(Ctx, "R RESTART", 14, (INTN)Ctx->Height - 18, 2, TRANSFORM_NORMAL, INFO_R, INFO_G, INFO_B);
  DrawText5x7(Ctx, "ESC EXIT", (INTN)Ctx->Width - 120, (INTN)Ctx->Height - 18, 2, TRANSFORM_NORMAL, INFO_R, INFO_G, INFO_B);
}

//
//  DrawCenteredBox
//
//  Dibuja un rectángulo con un borde directamente en el puro centro de la pantalla.
//

STATIC VOID DrawCenteredBox(const DRAW_CONTEXT *Ctx, UINTN BoxW, UINTN BoxH, UINT8 R, UINT8 G, UINT8 B)
{
  INTN X = ((INTN)Ctx->Width - (INTN)BoxW) / 2;
  INTN Y = ((INTN)Ctx->Height - (INTN)BoxH) / 2;

  FillRect(Ctx, X, Y, BoxW, BoxH, R, G, B);
  FillRect(Ctx, X + 6, Y + 6, BoxW - 12, BoxH - 12, BG_R, BG_G, BG_B);
}

//
//  GetRandomPosition
//
//  Calcula coordenadas X e Y pseudoaleatorias para tirar los nombres al inicio. 
//  Mide cuánto va a ocupar el texto en su estado más grande para asegurarse
//  de no elegir un centro que luego haga que las letras choquen con los bordes
//  al rotarlas. Usa la hora de UEFI como semilla.
//

STATIC VOID GetRandomPosition(const DRAW_CONTEXT *Ctx, const CHAR8 *Name1, const CHAR8 *Name2, UINTN Scale, MYNAME_TRANSFORM Transform, INTN *OutX, INTN *OutY)
{
  TEXT_EXTENTS T1 = MeasureText5x7(Name1, Scale, TRANSFORM_NORMAL);
  TEXT_EXTENTS T2 = MeasureText5x7(Name2, Scale, TRANSFORM_NORMAL);

  UINTN NormalW = (T1.Width > T2.Width ? T1.Width : T2.Width);
  UINTN NormalH = T1.Height + NAME_GAP + T2.Height;

  UINTN MaxDim = (NormalW > NormalH) ? NormalW : NormalH;
  UINTN HalfDim = MaxDim / 2;

  UINTN MinX = SCREEN_MARGIN + HalfDim;
  UINTN MaxX = (Ctx->Width > (SCREEN_MARGIN + HalfDim)) ? (Ctx->Width - SCREEN_MARGIN - HalfDim) : MinX;

  UINTN MinY = SCREEN_MARGIN + HalfDim;
  UINTN MaxY = (Ctx->Height > (SCREEN_MARGIN + HalfDim + 50)) ? (Ctx->Height - SCREEN_MARGIN - HalfDim - 50) : MinY;

  UINTN RangeX = (MaxX > MinX) ? (MaxX - MinX) : 1;
  UINTN RangeY = (MaxY > MinY) ? (MaxY - MinY) : 1;

  STATIC UINT32 RngState = 0;

  if (RngState == 0)
  {
    EFI_TIME Now;
    if (!EFI_ERROR(RT->GetTime(&Now, NULL)))
    {
      RngState = Now.Nanosecond ^ (Now.Second << 8) ^ (Now.Minute << 16) ^ (Now.Hour << 24) ^ 0x1A2B3C4D;
    }
    if (RngState == 0) RngState = 0x9E3779B9; 
  }

  RngState = (RngState * 1103515245 + 12345) & 0x7FFFFFFF;
  UINT32 RandX = RngState;

  RngState = (RngState * 1103515245 + 12345) & 0x7FFFFFFF;
  UINT32 RandY = RngState;

  *OutX = (INTN)(MinX + (RandX % RangeX));
  *OutY = (INTN)(MinY + (RandY % RangeY));
}

//
//  AsciiLen
//
//  Función manual para sacar el largo de un string, ya que en UEFI no
//  tenemos el strlen() normal de la librería de C.
//

STATIC UINTN AsciiLen(const CHAR8 *Str)
{
  UINTN Len = 0;
  while(Str[Len] != '\0')
  {
    Len++;
  }
  return Len;
}

//
//  MeasureText5x7
//
//  Calcula cuántos pixeles de ancho y de alto va a ocupar un texto entero
//  tomando en cuenta la escala, los espacios entre letras y la transformación
//  (si está acostado o de pie).
//

STATIC TEXT_EXTENTS MeasureText5x7(const CHAR8 *Text, UINTN Scale, MYNAME_TRANSFORM Transform)
{
  TEXT_EXTENTS T;
  UINTN Len = AsciiLen(Text);

  UINTN BaseW = 0;
  UINTN BaseH = 7 * Scale;

  if(Len > 0)
  {
    BaseW = Len * (5 * Scale) + (Len - 1) * (LETTER_SPACING * Scale);
  }

  if(Transform == TRANSFORM_ROT_LEFT_90 || Transform == TRANSFORM_ROT_RIGHT_90)
  {
    T.Width = BaseH;
    T.Height = BaseW;
  }
  else
  {
    T.Width = BaseW;
    T.Height = BaseH;
  }

  return T;
}

//
//  FindGlyph
//
//  Busca un caracter específico en nuestra tabla de fuente (FontTable).
//  Si le pasas una minúscula, la pasa a mayúscula automáticamente.
//  Si no encuentra el caracter, devuelve un espacio en blanco por defecto.
//

STATIC const FONT5X7 *FindGlyph(CHAR8 Ch)
{
  if(Ch >= 'a' && Ch <= 'z')
  {
    Ch = (CHAR8)(Ch - ('a' - 'A'));
  }

  for(UINTN i = 0; FontTable[i].c != '\0'; i++)
  {
    if(FontTable[i].c == Ch)
    {
      return &FontTable[i];
    }
  }

  return FindGlyph(' ');
}

//
//  DrawText5x7
//
//  Recorre un string letra por letra y llama a la función de dibujar
//  el caracter. Mueve el "cursor" imaginario dependiendo de la transformación, 
//  para que las letras se escriban de izquierda a derecha, de arriba abajo, etc.
//

STATIC VOID DrawText5x7(const DRAW_CONTEXT *Ctx, const CHAR8 *Text, INTN X, INTN Y, UINTN Scale, MYNAME_TRANSFORM Transform, UINT8 R, UINT8 G, UINT8 B)
{
  INTN CursorX = X;
  INTN CursorY = Y;

  TEXT_EXTENTS GlyphSize = MeasureText5x7("A", Scale, Transform);

  for(UINTN i = 0; Text[i] != '\0'; i++)
  {
    DrawGlyph5x7(Ctx, Text[i], CursorX, CursorY, Scale, Transform, R, G, B);

      if(Transform == TRANSFORM_ROT_LEFT_90)
      {
          CursorY -= (INTN)GlyphSize.Height + (INTN)(LETTER_SPACING * Scale);
      }
      else if(Transform == TRANSFORM_ROT_RIGHT_90)
      {
          CursorY += (INTN)GlyphSize.Height + (INTN)(LETTER_SPACING * Scale);
      }
      else
      {
          CursorX += (INTN)(5 * Scale) + (INTN)(LETTER_SPACING * Scale);
      }
  }
}

//
//  DrawGlyph5x7
//
//  Aquí es donde ocurre la magia de verdad. Revisa la matriz de 5x7 bits de la letra, 
//  y dependiendo de la transformación escogida, altera las coordenadas X y Y (DX, DY)
//  para voltear la letra patas arriba, rotarla, etc., antes de dibujar los cuadritos.
//

STATIC VOID DrawGlyph5x7(const DRAW_CONTEXT *Ctx, CHAR8 Ch, INTN X, INTN Y, UINTN Scale, MYNAME_TRANSFORM Transform, UINT8 R, UINT8 G, UINT8 B)
{
  const FONT5X7 *Glyph = FindGlyph(Ch);
  if(Glyph == NULL)
  {
    return;
  }

  const UINTN SrcW = 5;
  const UINTN SrcH = 7;

  for(UINTN Row = 0; Row < SrcH; Row++)
  {
    UINT8 Bits = Glyph->rows[Row];

    for(UINTN Col = 0; Col < SrcW; Col++)
    {
      if((Bits & (1 << (4 - Col))) == 0)
      {
        continue;
      }

      INTN DX = 0;
      INTN DY = 0;

      switch(Transform)
      {
        case TRANSFORM_NORMAL:
          DX = (INTN)Col;
          DY = (INTN)Row;
          break;

        case TRANSFORM_ROT_LEFT_90:
          DX = (INTN)Row;
          DY = (INTN)(SrcW - 1 - Col);
          break;

        case TRANSFORM_ROT_RIGHT_90:
          DX = (INTN)(SrcH - 1 - Row);
          DY = (INTN)Col;
          break;

        case TRANSFORM_FLIP_VERTICAL:
          DX = (INTN)Col;
          DY = (INTN)(SrcH - 1 - Row);
          break;

        case TRANSFORM_ROT_180:
          DX = (INTN)(SrcW - 1 - Col);
          DY = (INTN)(SrcH - 1 - Row);
          break;

        default:
          DX = (INTN)Col;
          DY = (INTN)Row;
          break;
      }

      FillRect(Ctx, X + DX * (INTN)Scale, Y + DY * (INTN)Scale, Scale, Scale, R, G, B);
    }
  }
}

//
//  ClearScreenColor
//
//  Llena toda la pantalla de un solo color sobreescribiendo el framebuffer 
//  fila por fila. Es mucho más rápido que pintar pixel por pixel.
//

STATIC VOID ClearScreenColor(const DRAW_CONTEXT *Ctx, UINT8 R, UINT8 G, UINT8 B)
{
  UINT32 Packed = PackColor(Ctx, R, G, B);

  for(UINT32 Y = 0; Y < Ctx->Height; Y++)
  {
    UINT32 *Row = Ctx->FrameBuffer + (UINTN)Y * Ctx->PixelsPerScanLine;
    for(UINT32 X = 0; X < Ctx->Width; X++)
    {
      Row[X] = Packed;
    }
  }
}

//
//  FillRect
//
//  Dibuja un rectángulo sólido iterando en un rango de pixeles en X y Y.
//  Tiene cheques de seguridad para no tratar de escribir fuera de la memoria 
//  de la pantalla y crashear la compu.
//

STATIC VOID FillRect(const DRAW_CONTEXT *Ctx, INTN X, INTN Y, UINTN W, UINTN H, UINT8 R, UINT8 G, UINT8 B)
{
  UINT32 Packed = PackColor(Ctx, R, G, B);

  for(UINTN J = 0; J < H; J++)
  {
    INTN PY = Y + (INTN)J;
    if(PY < 0 || PY >= (INTN)Ctx->Height)
    {
      continue;
    }

    UINT32 *Row = Ctx->FrameBuffer + (UINTN)PY * Ctx->PixelsPerScanLine;

    for(UINTN I = 0; I < W; I++)
    {
      INTN PX = X + (INTN)I;
      if(PX < 0 || PX >= (INTN)Ctx->Width)
      {
        continue;
      }

      Row[PX] = Packed;
    }
  }
}

//
//  PutPixel
//
//  Pone un puntito de color en una coordenada específica.
//

STATIC VOID PutPixel(const DRAW_CONTEXT *Ctx, INTN X, INTN Y, UINT8 R, UINT8 G, UINT8 B)
{
  if(X < 0 || Y < 0 || X >= (INTN)Ctx->Width || Y >= (INTN)Ctx->Height)
  {
    return;
  }

  Ctx->FrameBuffer[(UINTN)Y * Ctx->PixelsPerScanLine + (UINTN)X] = PackColor(Ctx, R, G, B);
}

//
//  PackColor
//
//  En UEFI cada compu puede guardar los colores de forma distinta en memoria. 
//  Algunas usan Rojo-Verde-Azul, otras Azul-Verde-Rojo. Esta función arma
//  el valor de 32 bits correcto dependiendo del formato que detectó GOP en el arranque.
//

STATIC UINT32 PackColor(const DRAW_CONTEXT *Ctx, UINT8 R, UINT8 G, UINT8 B)
{
  switch(Ctx->PixelFormat)
  {
    case PixelRedGreenBlueReserved8BitPerColor:
      return ((UINT32)R) | (((UINT32)G) << 8) | (((UINT32)B) << 16);

    case PixelBlueGreenRedReserved8BitPerColor:
      return ((UINT32)B) | (((UINT32)G) << 8) | (((UINT32)R) << 16);

    case PixelBitMask:
    {
      UINT32 Value = 0;

      if(Ctx->PixelInfo.RedMask)
      {
        UINTN Shift = 0;
        UINT32 Mask = Ctx->PixelInfo.RedMask;
        while(((Mask >> Shift) & 1U) == 0U) Shift++;
        Value |= (((UINT32)R << Shift) & Ctx->PixelInfo.RedMask);
      }

      if(Ctx->PixelInfo.GreenMask)
      {
        UINTN Shift = 0;
        UINT32 Mask = Ctx->PixelInfo.GreenMask;
        while(((Mask >> Shift) & 1U) == 0U) Shift++;
        Value |= (((UINT32)G << Shift) & Ctx->PixelInfo.GreenMask);
      }

      if(Ctx->PixelInfo.BlueMask)
      {
        UINTN Shift = 0;
        UINT32 Mask = Ctx->PixelInfo.BlueMask;
        while(((Mask >> Shift) & 1U) == 0U) Shift++;
        Value |= (((UINT32)B << Shift) & Ctx->PixelInfo.BlueMask);
      }

      return Value;
    }

    default:
      return ((UINT32)B) | (((UINT32)G) << 8) | (((UINT32)R) << 16);
  }
}
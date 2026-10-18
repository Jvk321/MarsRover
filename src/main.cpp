#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>

using namespace std;

int main(void)
{

  cout << "Wczytywanie danych z pliku\n";

  string nazwap = "grid.txt";
  int wym2 = 20;
  int wym1 = 20;

  // teraz deklarujemy dynamicznie tablice do, kt�rej wczytamyu nasz plik,
  int rows = wym2 + 1;
  double **G;
  G = new double *[rows];
  while (rows--)
    G[rows] = new double[wym1 + 1];

  cout << "\n\nNacisnij ENTER aby wczytac tablice o nazwie " << nazwap;
  getchar();

  std::ifstream plik(nazwap.c_str());

  for (unsigned int i = 1; i < wym2 + 1; i++)
  {
    for (unsigned int j = 1; j < wym1 + 1; j++)
    {
      plik >> G[i][j];
    }
  }
  plik.close();

  // Jeśli dwa pola mają takie samo f, wybieram pole dodane najpóźniej!
  // początek
  int x_start;
  int y_start;

  int x_cel;
  int y_cel;

  // komunikat -> podaj współrzędne, użytkownik podaje współrzędne startu i celu.
  cout << "Podaj dwie współrzędne x i y dla startu\n";
  cin >> x_start;
  cin >> y_start;

  cout << "Podaj dwie współrzędne x i y dla celu\n";
  cin >> x_cel;
  cin >> y_cel;

  // sprawdza czy współrzędne nie są poza mapą.
  if ((x_start < 0 || x_start > 19) ||
      (y_start < 0 || y_start > 19) ||
      (x_cel < 0 || x_cel > 19) ||
      (y_cel < 0 || y_cel > 19))
  {
    cout << "wspołrzędne poza mapą\n";
    return -1;
  }

  // przelicza współrzędne na prawidłowe. y to wiersz , x to kolumna

  int x2_start = x_start + 1;
  int y2_start = 20 - y_start;
  int x2_cel = x_cel + 1;
  int y2_cel = 20 - y_cel;

  const int miejsce_dostepne = 0;
  const int przeszkoda = 5;
  const int trasa = 3;
  const int koszt = 1;

  // sprawdzenie startu i celu, czy nie leżą na przeszkodzie.

  if (G[y2_start][x2_start] != przeszkoda)
  {
    cout << "start ok\n";
  }
  else
  {
    cout << "start na przeszkodzie\n";
    return -1;
  }

  if (G[y2_cel][x2_cel] != przeszkoda)
  {
    cout << "cel ok\n";
  }
  else
  {
    cout << "cel na przeszkodzie\n";
    return -1;
  }

  // aktualna pozycja
  int x_aktualny = x2_start;
  int y_aktualny = y2_start;

  // h(poz) to heurystyka szacująca odległość do celu z pozycji poz
  // h((poz_x,poz_y)) = sqrt{(poz_x - cel_x)^2 + (poz_y - cel_y)^2}
  // (poz_x, poz_y) to współrzędne kratki, (cel_x, cel_y) to współrzędne celu

  double poz_h;
  double cel_h;
  double h;

  poz_h = pow(x_aktualny - x2_cel, 2);
  cel_h = pow(y_aktualny - y2_cel, 2);
  h = sqrt(poz_h + cel_h);

  struct Kratka
  {
    int x;
    int y;
    double h;
    int g;
    double f;
    int rodzic_x;
    int rodzic_y;
  };

  vector<Kratka> lista_otwarta = {};
  vector<Kratka> lista_zamknieta = {};

  Kratka start;
  start.x = x_aktualny;
  start.y = y_aktualny;
  start.h = h;
  start.g = 0;
  start.f = h;
  start.rodzic_x = 0;
  start.rodzic_y = 0;

  lista_zamknieta.push_back(start);

  Kratka aktualna = start;

  bool cel_znaleziony = false;

  if (aktualna.x == x2_cel && aktualna.y == y2_cel)
  {
    cel_znaleziony = true;
  }

  while (cel_znaleziony != true)
  {
    // ruch góra sprawdzenie czy możliwy
    if ((aktualna.y - 1) >= 1 && (aktualna.y - 1) <= 20)
    {
      if (G[aktualna.y - 1][aktualna.x] != przeszkoda)
      {
        Kratka sasiad;
        sasiad.x = aktualna.x;
        sasiad.y = aktualna.y - 1;
        sasiad.h = sqrt(pow(sasiad.x - x2_cel, 2) + pow(sasiad.y - y2_cel, 2));
        sasiad.g = aktualna.g + koszt;
        sasiad.f = sasiad.g + sasiad.h;
        sasiad.rodzic_x = aktualna.x;
        sasiad.rodzic_y = aktualna.y;

        bool zamkniety = false;
        for (int i = 0; i < lista_zamknieta.size(); i++)
        {
          if (lista_zamknieta[i].x == sasiad.x && lista_zamknieta[i].y == sasiad.y)
          {
            zamkniety = true;
          }
        }
        if (zamkniety == false)
        {
          bool znaleziony = false;
          for (int i = 0; i < lista_otwarta.size(); i++)
          {
            if (lista_otwarta[i].x == sasiad.x && lista_otwarta[i].y == sasiad.y)
            {
              znaleziony = true;
              if (sasiad.f < lista_otwarta[i].f)
              {
                lista_otwarta[i].g = sasiad.g;
                lista_otwarta[i].h = sasiad.h;
                lista_otwarta[i].f = sasiad.f;
                lista_otwarta[i].rodzic_x = aktualna.x;
                lista_otwarta[i].rodzic_y = aktualna.y;
              }
            }
          }
          if (znaleziony == false)
          {
            lista_otwarta.push_back(sasiad);
          }
        }
      }
    }

    // ruch dół sprawdzenie czy możliwy
    if ((aktualna.y + 1) >= 1 && (aktualna.y + 1) <= 20)
    {
      if (G[aktualna.y + 1][aktualna.x] != przeszkoda)
      {
        Kratka sasiad;
        sasiad.x = aktualna.x;
        sasiad.y = aktualna.y + 1;
        sasiad.h = sqrt(pow(sasiad.x - x2_cel, 2) + pow(sasiad.y - y2_cel, 2));
        sasiad.g = aktualna.g + koszt;
        sasiad.f = sasiad.g + sasiad.h;
        sasiad.rodzic_x = aktualna.x;
        sasiad.rodzic_y = aktualna.y;

        bool zamkniety = false;
        for (int i = 0; i < lista_zamknieta.size(); i++)
        {
          if (lista_zamknieta[i].x == sasiad.x && lista_zamknieta[i].y == sasiad.y)
          {
            zamkniety = true;
          }
        }
        if (zamkniety == false)
        {
          bool znaleziony = false;
          for (int i = 0; i < lista_otwarta.size(); i++)
          {
            if (lista_otwarta[i].x == sasiad.x && lista_otwarta[i].y == sasiad.y)
            {
              znaleziony = true;
              if (sasiad.f < lista_otwarta[i].f)
              {
                lista_otwarta[i].g = sasiad.g;
                lista_otwarta[i].h = sasiad.h;
                lista_otwarta[i].f = sasiad.f;
                lista_otwarta[i].rodzic_x = aktualna.x;
                lista_otwarta[i].rodzic_y = aktualna.y;
              }
            }
          }
          if (znaleziony == false)
          {
            lista_otwarta.push_back(sasiad);
          }
        }
      }
    }

    // ruch lewo sprawdzenie czy możliwy
    if ((aktualna.x - 1) >= 1 && (aktualna.x - 1) <= 20)
    {
      if (G[aktualna.y][aktualna.x - 1] != przeszkoda)
      {
        Kratka sasiad;
        sasiad.x = aktualna.x - 1;
        sasiad.y = aktualna.y;
        sasiad.h = sqrt(pow(sasiad.x - x2_cel, 2) + pow(sasiad.y - y2_cel, 2));
        sasiad.g = aktualna.g + koszt;
        sasiad.f = sasiad.g + sasiad.h;
        sasiad.rodzic_x = aktualna.x;
        sasiad.rodzic_y = aktualna.y;

        bool zamkniety = false;
        for (int i = 0; i < lista_zamknieta.size(); i++)
        {
          if (lista_zamknieta[i].x == sasiad.x && lista_zamknieta[i].y == sasiad.y)
          {
            zamkniety = true;
          }
        }
        if (zamkniety == false)
        {
          bool znaleziony = false;
          for (int i = 0; i < lista_otwarta.size(); i++)
          {
            if (lista_otwarta[i].x == sasiad.x && lista_otwarta[i].y == sasiad.y)
            {
              znaleziony = true;
              if (sasiad.f < lista_otwarta[i].f)
              {
                lista_otwarta[i].g = sasiad.g;
                lista_otwarta[i].h = sasiad.h;
                lista_otwarta[i].f = sasiad.f;
                lista_otwarta[i].rodzic_x = aktualna.x;
                lista_otwarta[i].rodzic_y = aktualna.y;
              }
            }
          }
          if (znaleziony == false)
          {
            lista_otwarta.push_back(sasiad);
          }
        }
      }
    }

    // ruch prawo sprawdzenie czy możliwy
    if ((aktualna.x + 1) >= 1 && (aktualna.x + 1) <= 20)
    {
      if (G[aktualna.y][aktualna.x + 1] != przeszkoda)
      {
        Kratka sasiad;
        sasiad.x = aktualna.x + 1;
        sasiad.y = aktualna.y;
        sasiad.h = sqrt(pow(sasiad.x - x2_cel, 2) + pow(sasiad.y - y2_cel, 2));
        sasiad.g = aktualna.g + koszt;
        sasiad.f = sasiad.g + sasiad.h;
        sasiad.rodzic_x = aktualna.x;
        sasiad.rodzic_y = aktualna.y;

        bool zamkniety = false;
        for (int i = 0; i < lista_zamknieta.size(); i++)
        {
          if (lista_zamknieta[i].x == sasiad.x && lista_zamknieta[i].y == sasiad.y)
          {
            zamkniety = true;
          }
        }
        if (zamkniety == false)
        {
          bool znaleziony = false;
          for (int i = 0; i < lista_otwarta.size(); i++)
          {
            if (lista_otwarta[i].x == sasiad.x && lista_otwarta[i].y == sasiad.y)
            {
              znaleziony = true;
              if (sasiad.f < lista_otwarta[i].f)
              {
                lista_otwarta[i].g = sasiad.g;
                lista_otwarta[i].h = sasiad.h;
                lista_otwarta[i].f = sasiad.f;
                lista_otwarta[i].rodzic_x = aktualna.x;
                lista_otwarta[i].rodzic_y = aktualna.y;
              }
            }
          }
          if (znaleziony == false)
          {
            lista_otwarta.push_back(sasiad);
          }
        }
      }
    }

    if (lista_otwarta.size() != 0)
    {
      int index = 0;
      for (int i = 0; i < lista_otwarta.size(); i++)
      {
        if (lista_otwarta[i].f <= lista_otwarta[index].f)
        {
          index = i;
        }
      }
      aktualna = lista_otwarta[index];
      lista_otwarta.erase(lista_otwarta.begin() + index);
      lista_zamknieta.push_back(aktualna);
    }
    else
    {
      cout << "cel nie osiągnięty\n";
      break;
    }

    if (aktualna.x == x2_cel && aktualna.y == y2_cel)
    {
      cel_znaleziony = true;
    }
  }

  if (cel_znaleziony == true)
  {
    G[aktualna.y][aktualna.x] = trasa;
    while (x2_start != aktualna.x || y2_start != aktualna.y)
    {
      for (int i = 0; i < lista_zamknieta.size(); i++)
      {
        if (lista_zamknieta[i].x == aktualna.rodzic_x && lista_zamknieta[i].y == aktualna.rodzic_y)
        {
          aktualna = lista_zamknieta[i];
          break;
        }
      }
      G[aktualna.y][aktualna.x] = trasa;
    }
  }

  cout << "\nWypisujemy na ekran\n\n";
  for (int i = 1; i < wym2 + 1; i++)
  {
    for (int j = 1; j < wym1 + 1; j++)
    {
      cout << " " << G[i][j];
    }
    cout << "\n";
  }

  // na koniec czy�cimy pami�� po naszej tablicy
  for (int i = 0; i < wym2 + 1; i++)
  {
    delete[] G[i];
  }           // czyscimy wiersze
  delete[] G; // zwalniamy tablice wskaznikow do wierszy

  cout << "\n\nNacisnij ENTER aby zakonczyc";
  getchar();

  return 0;
}
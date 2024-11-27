#ifndef GRAPH_H
#define GRAPH_H

// Program to check if a given directed graph is strongly connected or not
#include <iostream>
#include <list>
#include <stack>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include <ctime>
#include <sstream>
#define _USE_MATH_DEFINES
#include <cmath>
#include <deque>
#include <float.h>
#include <functional>
#include "paths.h"
#include <time.h>
#include <ctime>

using namespace std;

// definición de constantes
const double RAD = 6371.0088;       //radio de la Tierra considerado
const int NDEC = 3;
const int NDEC2 = 1;

//Algunos atributos de los camiones
//const double CapKilosTrucks = 25000;
const double Factor = 1.0;              // esto es para calibrar la capacidad máxima que voy aceptar en peso de los pallets, para poder armarlos de a pares
//const double CapPalletsT1 = 26;
//const double CapPalletsT2 = 12;

const double UmbralPesoCaja = 800;
const double UmbralMinimoAmount = 0.01; //chequea si hay error
bool CUAD = true;                // true si se corre modelo con cuadratura, false si es sin cuadratura
const bool cajas_enteras = true;     // true si es con cajas var enteras, false si aguanta var continuas
const bool Ascendente = true;       // si es true considera primero los valores más pequeños de cajas por pallets (para cargar pallets, si es false es al revés
const size_t ITER_GRASP = 500;   //ACÁ TENGO 500
const int PGRASP = 3;
const int NITERSP = 100;// número de iteraciones máximas para tratar de cargar pallets no cargados (100)
const int NITERDES = 5;
const int MaxIterAjuste = 50; // numero máximo de iteraciones para tratar de traspasar dos pallets congelados de un camión a otro
const size_t MAXIterFam = 10;
const int ITERAJUSTECong = 5;      // número maximo de iteraciones para ajustar restriccion de congelados, default 5
const int NOPTIONS = 10;    // opciones de descarga para arreglar pallets no cargados
const double DeltaTime = 6; // 24 - delta es el umbral de horas que tengo para programar next day

const bool COND_DESARMA = true; //true si decido sacar cajas, false si no
const double F1 = 0.5;      //factor de reducción si se me queda un pallet abajo
const double F2 = 0.75;     //factor de reducción 2 o más pallets abajo (más raro que pase esto)
const double F3 = 0.5;      //máxima reducción de tamaño
const int NITERPROG = 10;

const bool NoFamilia = true;

const int COND_CONGELADO = 2;   //0 si trata de llenar camiones completos con congelado y al final reparte fresco; 1 si se reparte más parejo; 2 llena de congelado los camiones en orden

struct Prod_parts
{
    string Codigo;       // nodo en que ese producto es producido en caso de Precio, codigo de producto en caso de demanda
    double amount;      // se usa para precio, y o, u
    bool Supermercado;  // se usa para demanda, true si la demanda es en supermercado, false si es precio o demanda no en supermercado
    int frescura;
    
    Prod_parts(string t, double am, bool super): Codigo(t), amount(am), Supermercado(super), frescura(-1) {}
};

struct Transporte {
    string  tipo;
    int peso;
    int volumen;
    int minP;
    int maxP;
};

// class Producto
class Producto
{
private:
    int id;                 // id del producto en vector de producto
    string codigo;         // string que me indica codigo del producto
    double Cfijo;
    double PesoCaja;        // peso en kilos de cada caja de este tipo de producto
    double CajasPallet;      // cantidad de cajas por pallet
    int familia;            // id en vector de famila a la que pertenece el producto
	string sector;
	string estado;
    
public:
	Producto(int idd, string codeP, string PCaja, string CPallets, string sector, string estado) : id(idd), codigo(codeP), Cfijo(0), PesoCaja(stod(PCaja)), CajasPallet(stod(CPallets)), sector(sector), estado(estado), familia(-1) {}
    vector<Prod_parts> Precio;      // vector de precios de este producto en varias plantas
    int getid() {return id;};
    string get_code() {return codigo;}
    double get_cfijo() {return Cfijo;}
    double get_PesoCaja() {return PesoCaja;}
    double get_CajasPallet() {return CajasPallet;}
    void ReadInfoCF(double CF) { Cfijo = CF;}
     int get_familia() {return familia;}
    void set_familia(int i) {familia = i;}
    double get_price(string Code);
	string get_sector() { return sector; }
	string get_estado() { return estado; }
};

double Producto::get_price(string Code)
{
    if(Precio.size()>0)
    {
        for(unsigned int i=0; i<Precio.size(); i++)
        {
            if(Precio[i].Codigo == Code)
                return Precio[i].amount;
        }
        return(Precio[0].amount);
    }       // end if
    else
        return 0;   // no se reporta precio de venta para ese producto
}                   // end get price

int get_Prod_id(string CODE, vector<Producto>& Prod)
{
    for(unsigned int k = 0; k < Prod.size(); k++)
    {
        if(CODE.compare(Prod[k].get_code()) == 0)
            return(k);
    }
    return(-1);     //codigo ID no fue encontrado...acá hay un error
}

// class familia
class Familia
{
private:
    vector<string> productos;
    int id;
public:
    Familia(int idd):id(idd) {congelado = false;}
    
    bool congelado;             //true si familia tiene productos congelados slamente, false si es familia de frescos
    int get_id() {return id;}
    void addp(string p)
    { productos.push_back(p); }
    bool belong(string pr)
    {
        for(unsigned int i=0; i<productos.size(); i++)
            if(productos[i]==pr)
                return(true);
        return(false);
    };
    void set_congelado(vector<Producto>& Prod);
    string PrintFamilias();
};

//la familia congelado significa que primer producto que pertenece es congelado
void Familia::set_congelado(vector<Producto>& Prod)
{
   if(productos.size()>0)
   {
       for(unsigned int i=0; i<productos.size(); i++)
       {
           int IDP = get_Prod_id(productos[0],Prod);
           if(Prod[IDP].get_estado() == "Congelado")
           {
               congelado = true;
               break;
           }
       }
   }
}       // end set_congelado

// imprime los id de productos por familia
string Familia::PrintFamilias()
{
    stringstream s;
    s << "Familia " << id << ";";
    if(congelado)
        s << "Congelado:";
    else
        s << "Fresco:";
    for(unsigned  int k= 0; k<productos.size(); k++)
        s << productos[k] << ";";
    s << endl;
    return s.str();
}

// clase Bunch_cajas
class Bunch_cajas
{
private:
    int id_producto;        //id de producto en vector de productos
    double n_cajas;         // número de cajas de ese producto
    int frescura;
    bool super;         // true si es super, false si no
    int descarga;       //solo si se crea de xs, e indica si se descarga en primera o segunda sucursal
    
public:
    Bunch_cajas(int ip, double nc, int fr, bool sp, bool cu, int des): id_producto(ip), n_cajas(nc), frescura(fr), super(sp), CUAD(cu), descarga(des) {}
   
    bool CUAD;                               // true si flujo proviene de cuadratura, false por defecto
    int get_idP() {return id_producto;}
    double Peso_bunch(vector<Producto>& Prod)
     { return Prod[id_producto].get_PesoCaja()*n_cajas;}
    string getProduct(vector<Producto>& Prod) {return Prod[id_producto].get_code();}
    string getSector(vector<Producto>& Prod) {return Prod[id_producto].get_sector();}
    double getCajas() {return n_cajas;}
    int getFrescura() {return frescura;}
    bool getSuper() {return super;}
    int getCUAD() {if(CUAD) return(1); else return 0;}
    string getEstado(vector<Producto>& Prod)
    { return Prod[id_producto].get_estado(); }
    double capac_prod(vector<Producto>& Prod)
    { return Prod[id_producto].get_CajasPallet();}
    void Add_cajas(double ExtraCajas)
    { n_cajas += ExtraCajas;}

 
    double ocupBunch(vector<Producto>& Prod) {
        return ((double) n_cajas / (double) capac_prod(Prod));
    }
    
    double pesoBunch(vector<Producto>& Prod) {
        return(n_cajas*Prod[id_producto].get_PesoCaja());
    }
    
    //fn que genera un bunch de cajas con lo que se decide remover y actualiza n_cajas
    Bunch_cajas desarma_bunch(double cajas_removidas)
    {
        Bunch_cajas auxiliar(id_producto,0,frescura,super,CUAD,descarga);
        n_cajas -= cajas_removidas;
        if(n_cajas < 0)
            n_cajas = 0;
        auxiliar.Add_cajas(cajas_removidas);
        return(auxiliar);
    }
    
    string PrintBunch(vector<Producto>& Prod)
    {
        stringstream s;
        if(n_cajas > 0)
        {
            s << "Bunch ;" << Prod[id_producto].get_code() << ";" << Prod[id_producto].get_sector() << ";" << Prod[id_producto].get_estado() << ";" << n_cajas << ";" << frescura << ";" << super << ";" << Peso_bunch(Prod) << ";" << capac_prod(Prod) << ";" << ocupBunch(Prod) << ";" << CUAD << ";" << descarga << ";[" << Prod[id_producto].get_familia() << "]" << endl;
        }
        return s.str();
    }
};

bool CompareBCUAD(const Bunch_cajas &B1, const Bunch_cajas &B2)
{ return B1.CUAD > B2.CUAD; }

// clase pallet
class Pallet
{
private:
    int id_pallet;              // identifica el pallet correspondiente (es un id único por pallet, no corresponde a la posición del pallet en ninguna estructura
    int id_camion;              // id de camion que mueve este pallet en arco correspondiente (es un id único por camion, no corresponde a la posición del camión en ninguna estructura
    int prev_edge;
    int next_edge;
    int descarga;   //-1 para todos, excepto aquellos que vienen de zs, 1 si se descargan en el arco propio, 2 si siguen hasta arco S-S
    
public:
    vector<Bunch_cajas> BC;     // vector con grupos de caja de cada producto
    double ocupacion;
    double peso;
    int familia;
    Pallet(int ip, int f, int pe, int se, int des): id_pallet(ip), id_camion(-1), familia(f), ocupacion(0), peso(0), prev_edge(pe), next_edge(se), descarga(des) {}
    
    double nCajas();    //número de cajas que tiene el pallet de varios productos
    void OcupPesoCuad(vector<Producto>& Prod);      //ocupación del pallet de varios productos (fracción ente 0 y 1)
    double ParteSector(string sector, vector<Producto>& Prod);      //obtiene la parte del pallet que es ocupada por sector
    double ParteSectorKilos(string sector, vector<Producto>& Prod);
    void set_camion(int cid) {id_camion = cid; }
    void change_adjacents(int pr, int nx) {prev_edge = pr; next_edge = nx;}
    int getid_pallet() {return id_pallet;}
    int get_prev_edge() {return prev_edge;}
    int get_next_edge() {return next_edge;}
    int get_descarga() {return descarga;}
    double CUAD;      // va entre 0 y 1
    double IndCUAD;     //potencial de CUAD = peso*CUAD
    
    void desarma_pallet(double pesoTarget, vector<Bunch_cajas>& cajasBorradas, vector<Producto>& Prod);
    void desarma_pallet2(double pesoTarget, vector<Bunch_cajas>& cajasBorradas, vector<Producto>& Prod);
    bool cargaExtra_pallet(Bunch_cajas BC, vector<Producto>& Prod);
    string Print_pallet(vector<Producto>& Prod);
    
};
// funciones para ordenar los Pallets de menor a mayor en ocupación, y de mayor a menor en Peso (para dos objetivos distintos)
bool CompareOcup(const Pallet &P1, const Pallet &P2)
{    return P1.ocupacion < P2.ocupacion; }

bool ComparePeso(const Pallet &P1, const Pallet &P2)
{    return P1.peso > P2.peso; }

bool CompareCUAD(const Pallet &P1, const Pallet &P2)
{ return P1.IndCUAD > P2.IndCUAD; }

bool CompareFam(const Pallet &P1, const Pallet &P2) //permite agrupar familias en vector de pallets
{
    if(NoFamilia)
        return(P1.peso > P2.peso);
    else
        return P1.familia < P2.familia || (P1.familia == P2.familia && P1.peso > P2.peso);
}

double Pallet::nCajas()
{
    double NAcum = 0;
    if(BC.size() > 0)
    {
        for(unsigned int k=0; k<BC.size(); k++)
            NAcum += BC[k].getCajas();
    }
    return(NAcum);
}
// actualiza datos de ocupación, peso y cuadratura
void Pallet::OcupPesoCuad(vector<Producto>& Prod)
{
    double Ocup = 0;
    double CA = 0;
    double pAcum = 0;
    double CajasTotales = 0;
    
    if(BC.size() > 0)
    {
        for(unsigned int k=0; k<BC.size(); k++)
        {
            pAcum += BC[k].Peso_bunch(Prod);
            Ocup += (double)BC[k].getCajas()/(double)BC[k].capac_prod(Prod);
            CajasTotales += BC[k].getCajas();
            CA += BC[k].getCUAD()*BC[k].getCajas();
        }
    }
    ocupacion = Ocup;
    CUAD = (double)CA/(double)CajasTotales;
    peso = pAcum;
    IndCUAD = CUAD*pAcum;
}


double Pallet::ParteSector(string sector, vector<Producto>& Prod)
{
    double parte = 0;
    for(unsigned int k=0; k<BC.size(); k++)
    {
        if(BC[k].getSector(Prod) == sector)
            parte += BC[k].ocupBunch(Prod);
    }       // end for
    return(parte);
}

double Pallet::ParteSectorKilos(string sector, vector<Producto>& Prod)
{
    double parte = 0;
    for(unsigned int k=0; k<BC.size(); k++)
    {
        if(BC[k].getSector(Prod) == sector)
            parte += BC[k].pesoBunch(Prod);
    }       // end for
    return(parte);
}

void DeleteAllBunch(vector<Bunch_cajas>& data, const vector<int>& deleteIndices)
{
    vector<bool> markedElements(data.size(), false);
    vector<Bunch_cajas> tempBuffer;
    tempBuffer.reserve(data.size()-deleteIndices.size());

    for (vector<int>::const_iterator itDel = deleteIndices.begin(); itDel != deleteIndices.end(); itDel++)
        markedElements[*itDel] = true;

    for (size_t i=0; i<data.size(); i++)
    {
        if (!markedElements[i])
            tempBuffer.push_back(data[i]);
    }
    data = tempBuffer;
}

void Pallet::desarma_pallet2(double pesoTarget, vector<Bunch_cajas>& cajasBorradas, vector<Producto>& Prod)
{
    OcupPesoCuad(Prod);
    
    vector<int> Lista_Borrar;
    Lista_Borrar.clear();
    double PesoAux = peso;
    double PesoAdd = 0;
    
    sort(BC.begin(),BC.end(),&CompareBCUAD);
    
    for(unsigned int k=0; k< BC.size(); k++)
    {
        double faltante = peso - pesoTarget;
        
        if(BC[k].pesoBunch(Prod) <= faltante)
        {
            cajasBorradas.push_back(BC[k]);
            Lista_Borrar.push_back(k);
            PesoAdd += BC[k].Peso_bunch(Prod);
        }
        else{

            double cajas_por_remover = floor((double)faltante/(double)Prod[BC[k].get_idP()].get_PesoCaja());
            Bunch_cajas aux = BC[k].desarma_bunch(cajas_por_remover);
            cajasBorradas.push_back(aux);

        }   // end else
        
        OcupPesoCuad(Prod);
        PesoAux = peso - PesoAdd;
        
        if(PesoAux < pesoTarget)
            break;
    }       // end for recorriendo BC
    //ahora hay que borrar los BC que se eliminaron completos
    if(Lista_Borrar.size() > 0)
        DeleteAllBunch(BC, Lista_Borrar);

}   // end desarma pallet2

void Pallet::desarma_pallet(double pesoTarget, vector<Bunch_cajas>& cajasBorradas, vector<Producto>& Prod)
{

    OcupPesoCuad(Prod);
    
    bool cond = true;
    bool encuentraCuad = true;
    int nit = 0;
    while(cond)
    {
        for(unsigned int k=0; k< BC.size(); k++)
        {
            if((BC[k].getCUAD() == 1)||(encuentraCuad == false)||(BC.size() == 1))
            {
                double faltante = peso - pesoTarget;
                
                if(BC[k].pesoBunch(Prod) <= faltante)
                {
                    cajasBorradas.push_back(BC[k]);
                    BC.erase(BC.begin()+k);
                }
                else{
                                        
                    double cajas_por_remover = floor((double)faltante/(double)Prod[BC[k].get_idP()].get_PesoCaja());
                    Bunch_cajas aux = BC[k].desarma_bunch(cajas_por_remover);
                    cajasBorradas.push_back(aux);
                    
                }
                break;
            }       // end if
            if(k==(BC.size()-1))
                encuentraCuad = false;
        }
        OcupPesoCuad(Prod);
                
        if(peso < pesoTarget)
            break;
        if(nit >= 2*BC.size())
            cond = false;
        nit++;
    }       // end while
    
}       // end desarma pallet
 
//me cabe o no me cabe el BC adicional que quiero subir
bool Pallet::cargaExtra_pallet(Bunch_cajas BC, vector<Producto>& Prod)
{
    OcupPesoCuad(Prod);
    double ExtraOcup = (double)BC.getCajas()/(double)BC.capac_prod(Prod);
    double OcFinal = ocupacion + ExtraOcup;
    if(OcFinal > 1)
        return(false);
    else
        return(true);
}   // end fn

string Pallet::Print_pallet(vector<Producto>& Prod)
{
    stringstream s;
    OcupPesoCuad(Prod);
    s << "(" << id_pallet << ";" << id_camion << ";" << ocupacion << ";" << peso << ";" << nCajas() << ";" << CUAD << ";" << prev_edge  << ";" << next_edge << ";" << descarga << ")[" << familia << "]" << endl;
    if(BC.size()>0)
    {
        for(unsigned int i=0; i<BC.size(); i++)
            s << BC[i].PrintBunch(Prod);
    }           // end if
    s << endl;
    return s.str();
}

struct trioPallet     // para reportar la carga de camion (id de familia 0 a 4, y id de Pallet a ser cargado y su peso)
{
    int fam; int idPallet; double peso;
    trioPallet(int f, int p, double pp): fam(f), idPallet(p), peso(pp) {}
    string print_pair()
    {
        stringstream ss;
        ss << fam << ";" << idPallet << ";" << peso << ";" << endl;
        return ss.str();
    }        // end print_solution
    
    bool operator< (const trioPallet &other) const {
        return peso > other.peso; }
};

bool CompareFamTrio(const trioPallet &P1, const trioPallet &P2) //permite agrupar familias en vector de pallets
{    return P1.fam < P2.fam || (P1.fam == P2.fam && P1.peso > P2.peso); }

// clase Camion
class Camion
{
private:
    int id_camion;  // id de camion que mueve este pallet en arco correspondiente (es un id único por camion, no corresponde a la posición del camión en ninguna estructura
//    int type;   // 1 de 26 pallets, 2 de 12 pallets
    string stype;   // 26P, 12P
    int id_edge;    // edge en lista de edges que sirve este camion
    int prev_edge;
    int next_edge;
public:
    string ClasifCamion;      // primario o interplanta
    bool Fresco1capa;
    int capacP;
    double capacK;
    int minPalletsC;    //mínima cantidad de Pallets congelados que soporta este tipo de camión si es compartido
    int maxPalletsC;    //idem pero cota máxima
    
    double TTotal;  //estimación de tiempo total requerido por este camión en cpmpletar su ruta
   
    Camion (int id) : id_camion(id), stype("dummy"), id_edge(-1), prev_edge(-1), next_edge(-1) {}
    Camion(int id, string t, int ed, int pe, int se, string ClCam, size_t NF, int capP, int capK, int minP, int maxP) : id_camion(id), stype(t), id_edge(ed), prev_edge(pe), next_edge(se), ClasifCamion(ClCam), capacP(capP), capacK(capK), minPalletsC(minP), maxPalletsC(maxP), TTotal(0)
    {
                
        Fresco1capa = false;
        carga.clear();

        computePesoOcupacion();
        for(unsigned int k=0; k<NF; k++)
            PalletsPorFamilia.push_back(0);
    }
    vector<Pallet> carga;       //tiene los pallets asociados a cada camión
    vector<int> PalletsPorFamilia;  // da cuenta de cuantos pallets tengo por cada familia en carga
    int get_idc() {return id_camion;}
    int get_prev() {return prev_edge;}
    int get_nxt() {return next_edge;}
    string get_stype() {return stype;}
    double ocupacion;
    double peso;
    int nPallets();
    void computePesoOcupacion();
    void change_adjacents(int pr, int nx) {prev_edge = pr; next_edge = nx;}
    // funciones para chequear si es posible meter más carga en pallets o kilos al camión
    bool sobreCargaP(int ExtraP);
    bool sobreCargaK(double ExtraK, vector<Producto>& Prod);
    
    int nPalletsC(vector<Familia>& FA);        //output el número de número de pallets congelados cargados en el camión en cada instante
    bool separador(vector<Familia>& FA);
    vector<double> palletsPorSector(vector<Pallet>& cargaN, vector<Familia>& FA, vector<Producto>& Prod);
    vector<double> KilosPorSector(vector<Pallet>& cargaN, vector<Familia>& FA, vector<Producto>& Prod);
    string print_camion();
    string print_camionDetails(vector<Producto>& Prod);
};

void Camion::computePesoOcupacion()
{
    double PT = 0;
    if(carga.size() > 0)
    {
        for(unsigned int i=0; i<carga.size(); i++)
            PT += carga[i].peso;
    }
    ocupacion = (double)PT/(double)capacK;
    peso = PT;
}

int Camion::nPallets()
{ return (int) carga.size(); }

bool Camion::sobreCargaP(int ExtraP)
{
    if(capacP >= (nPallets() + ExtraP))
        return(false);
    else return(true);
}
bool Camion::sobreCargaK(double ExtraK, vector<Producto>& Prod)
{
    computePesoOcupacion();
    if(capacK >= (peso + ExtraK))
        return(false);
    else return(true);
}

int Camion::nPalletsC(vector<Familia>& FA)
{
    int nC = 0;
    for(unsigned int k=0; k<carga.size(); k++)
    {
        if(FA[carga[k].familia].congelado)
            nC++;
    }       // end for
    return(nC);
}

bool Camion::separador(vector<Familia>& FA)        // indica si tiene separador o no el camión
{
    int Dif = nPallets() - nPalletsC(FA);
    if((Dif > 0)&&(Dif < nPallets()))
        return(true);
    else
        return(false);
}   //end separador

//fn que entrega la desagregación de archivo SAP transportes detalle
vector<double> Camion::KilosPorSector(vector<Pallet>& cargaN,  vector<Familia>& FA, vector<Producto>& Prod)
{
    vector<double> Aux; //acá guardo la secuencia de pallets clasificados
    Aux.clear();
    for(unsigned int i=0; i<13; i++)
        Aux.push_back(0);
    for(unsigned int k=0; k<cargaN.size(); k++)
    {
        if(FA[cargaN[k].familia].congelado)       //se guarda en un orden
        {
            double PolloCongelado = cargaN[k].ParteSectorKilos("Pollo", Prod);
            double PavoCongelado = cargaN[k].ParteSectorKilos("Pavo", Prod);
            double CerdoCongelado = cargaN[k].ParteSectorKilos("Cerdo", Prod);
            double ElaboradoCongelado = cargaN[k].ParteSectorKilos("Elaborado", Prod);
            double HortalizasCongelado = cargaN[k].ParteSectorKilos("Hortalizas y Frutas", Prod);
            double SalmonCongelado = cargaN[k].ParteSectorKilos("Salmón", Prod);
            
            //ahora se van sumando en Aux
            Aux[0] += PolloCongelado;
            Aux[1] += CerdoCongelado;
            Aux[2] += ElaboradoCongelado;
            Aux[3] += SalmonCongelado;
            Aux[4] += PavoCongelado;
            Aux[5] += HortalizasCongelado;
        }
        else                            //frescos
        {
            double Cecina = cargaN[k].ParteSectorKilos("Cecina", Prod);
            double PolloFresco = cargaN[k].ParteSectorKilos("Pollo", Prod);
            double PavoFresco = cargaN[k].ParteSectorKilos("Pavo", Prod);
            double CerdoFresco = cargaN[k].ParteSectorKilos("Cerdo", Prod);
            double SalmonFresco = cargaN[k].ParteSectorKilos("Salmón", Prod);
            
            //ahora se van sumando en Aux
            Aux[6] += PolloFresco;
            Aux[7] += CerdoFresco;
            Aux[8] += PavoFresco;
            Aux[9] += Cecina;
            Aux[10] += SalmonFresco;
        }
    }       // end for
    
    return(Aux);
    
}   //end palletsPorSector
//fn que entrega la desagregación de archivo SAP transportes detalle
vector<double> Camion::palletsPorSector(vector<Pallet>& cargaN, vector<Familia>& FA, vector<Producto>& Prod)
{
    vector<double> Aux; //acá guardo la secuencia de pallets clasificados
    Aux.clear();
    for(unsigned int i=0; i<13; i++)
        Aux.push_back(0);
    for(unsigned int k=0; k<cargaN.size(); k++)
    {
        if(FA[cargaN[k].familia].congelado)       //se guarda en un orden
        {
            double PolloCongelado = cargaN[k].ParteSector("Pollo", Prod);
            double PavoCongelado = cargaN[k].ParteSector("Pavo", Prod);
            double CerdoCongelado = cargaN[k].ParteSector("Cerdo", Prod);
            double ElaboradoCongelado = cargaN[k].ParteSector("Elaborado", Prod);
            double HortalizasCongelado = cargaN[k].ParteSector("Hortalizas y Frutas", Prod);
            double SalmonCongelado = cargaN[k].ParteSector("Salmón", Prod);
            
            //ahora se van sumando en Aux
            Aux[0] += PolloCongelado;
            Aux[1] += CerdoCongelado;
            Aux[2] += ElaboradoCongelado;
            Aux[3] += SalmonCongelado;
            Aux[4] += PavoCongelado;
            Aux[5] += HortalizasCongelado;
        }
        else                            //frescos
        {
            double Cecina = cargaN[k].ParteSector("Cecina", Prod);
            double PolloFresco = cargaN[k].ParteSector("Pollo", Prod);
            double PavoFresco = cargaN[k].ParteSector("Pavo", Prod);
            double CerdoFresco = cargaN[k].ParteSector("Cerdo", Prod);
            double SalmonFresco = cargaN[k].ParteSector("Salmón", Prod);
            
            //ahora se van sumando en Aux
            Aux[6] += PolloFresco;
            Aux[7] += CerdoFresco;
            Aux[8] += PavoFresco;
            Aux[9] += Cecina;
            Aux[10] += SalmonFresco;
        }
    }       // end for
    
    return(Aux);
    
}   //end palletsPorSector

string Camion::print_camion()
{
    stringstream s;
    s << "CAMION (" << id_camion << ";" << stype << ";" << ocupacion << ";" << TTotal << ";" << prev_edge << ";" << next_edge;
    for(unsigned int k=0; k<PalletsPorFamilia.size();k++)
        s << ";" << PalletsPorFamilia[k];
    s << ";" << nPallets() << ")";
    return s.str();
}

string Camion::print_camionDetails(vector<Producto>& Prod)
{
    stringstream s;
    s << "CAMION (" << id_camion << ";" << stype << ";" << ocupacion << ";" << TTotal << ";" << prev_edge << ";" << next_edge;
    for(unsigned int k=0; k<PalletsPorFamilia.size();k++)
        s << ";" << PalletsPorFamilia[k];
    s << ";" << nPallets() << ")" << endl;
    s << "Detalle de Pallets cargados" << endl;
    s << endl;
    for(unsigned int i=0; i<carga.size(); i++)
        s << carga[i].Print_pallet(Prod);
    
    return s.str();
}

// función para ordenar de más chico a más grande vector de camiones por ocupación en peso, partiendo por camiones más grandes
bool CompareCamion(const Camion &C1, const Camion &C2)
{
    return C1.capacP > C2.capacP || (C1.capacP == C2.capacP && C1.peso < C2.peso);      // para ordenar de mayor a menor en peso cargado, podría cambiarse por ocupación en número de pallets, aparentemente los resultados son parecidos
}

//funciones que se generan para ordenar dejando primero a los que tienen más pallets de una cierta familia (acá se construye para máxino de 10 familias)
bool CompareCamionF0(const Camion &C1, const Camion &C2)
{ return C1.PalletsPorFamilia[0] > C2.PalletsPorFamilia[0] || (C1.PalletsPorFamilia[0] == C2.PalletsPorFamilia[0] && (C1.capacP > C2.capacP || (C1.capacP == C2.capacP && C1.peso < C2.peso)));}
bool CompareCamionF1(const Camion &C1, const Camion &C2)
{ return C1.PalletsPorFamilia[1] > C2.PalletsPorFamilia[1] || (C1.PalletsPorFamilia[1] == C2.PalletsPorFamilia[1] && (C1.capacP > C2.capacP || (C1.capacP == C2.capacP && C1.peso < C2.peso)));}
bool CompareCamionF2(const Camion &C1, const Camion &C2)
{ return C1.PalletsPorFamilia[2] > C2.PalletsPorFamilia[2] || (C1.PalletsPorFamilia[2] == C2.PalletsPorFamilia[2] && (C1.capacP > C2.capacP || (C1.capacP == C2.capacP && C1.peso < C2.peso)));}
bool CompareCamionF3(const Camion &C1, const Camion &C2)
{ return C1.PalletsPorFamilia[3] > C2.PalletsPorFamilia[3] || (C1.PalletsPorFamilia[3] == C2.PalletsPorFamilia[3] && (C1.capacP > C2.capacP || (C1.capacP == C2.capacP && C1.peso < C2.peso)));}
bool CompareCamionF4(const Camion &C1, const Camion &C2)
{ return C1.PalletsPorFamilia[4] > C2.PalletsPorFamilia[4] || (C1.PalletsPorFamilia[4] == C2.PalletsPorFamilia[4] && (C1.capacP > C2.capacP || (C1.capacP == C2.capacP && C1.peso < C2.peso)));}
bool CompareCamionF5(const Camion &C1, const Camion &C2)
{ return C1.PalletsPorFamilia[5] > C2.PalletsPorFamilia[5] || (C1.PalletsPorFamilia[5] == C2.PalletsPorFamilia[5] && (C1.capacP > C2.capacP || (C1.capacP == C2.capacP && C1.peso < C2.peso)));}
bool CompareCamionF6(const Camion &C1, const Camion &C2)
{ return C1.PalletsPorFamilia[6] > C2.PalletsPorFamilia[6] || (C1.PalletsPorFamilia[6] == C2.PalletsPorFamilia[6] && (C1.capacP > C2.capacP || (C1.capacP == C2.capacP && C1.peso < C2.peso)));}
bool CompareCamionF7(const Camion &C1, const Camion &C2)
{ return C1.PalletsPorFamilia[7] > C2.PalletsPorFamilia[7] || (C1.PalletsPorFamilia[7] == C2.PalletsPorFamilia[7] && (C1.capacP > C2.capacP || (C1.capacP == C2.capacP && C1.peso < C2.peso)));}
bool CompareCamionF8(const Camion &C1, const Camion &C2)
{ return C1.PalletsPorFamilia[8] > C2.PalletsPorFamilia[8] || (C1.PalletsPorFamilia[8] == C2.PalletsPorFamilia[8] && (C1.capacP > C2.capacP || (C1.capacP == C2.capacP && C1.peso < C2.peso)));}
bool CompareCamionF9(const Camion &C1, const Camion &C2)
{ return C1.PalletsPorFamilia[9] > C2.PalletsPorFamilia[9] || (C1.PalletsPorFamilia[9] == C2.PalletsPorFamilia[9] && (C1.capacP > C2.capacP || (C1.capacP == C2.capacP && C1.peso < C2.peso)));}

//chequea si es posible cargar pallets congelados en la segunda etapa del viaje, treu si trae fresco, false si no
bool TraeFresco(Camion cam, vector<Familia>& FA)
{
    if(cam.carga.size() > 0)
    {
        for(unsigned int i=0; i<cam.carga.size(); i++)
        {
            if(!FA[cam.carga[i].familia].congelado)
                return(true);      // no se puede cargar este pallet en este camión
        }
    }
    return(false);
}

/*string time_to_local_date(time_t utc)
{
    time_t utc2  = utc + 3*3600;
    struct tm *now = localtime(&utc2);
    char buffer[80];
    strftime(buffer, 80, "%d/%m/%Y %I:%M%p", now);
    return string(buffer);
}*/

//me identifica donde están los camiones
struct camion_identificador
{
    int camion_id;      // id de camión identificado
    int edge_id;        // id de edge asociado a schedule de camión
    int CamArray_type;    // 1 si camion está en CamionesD, 2 si está en Camiones2, y 3 si está en CamionesS
    int CamArray_id;       // id dentro de vector de tipo definido arriba
    int CamArray_id2;       // id segunda capa en caso de ser Camiones2 o CamionesS, -1 en caso de CamionesD
    
    camion_identificador(int ci, int ei, int ct, int cid, int cid2): camion_id(ci),edge_id(ei), CamArray_type(ct), CamArray_id(cid), CamArray_id2(cid2) {}
};

struct trioProgramacion     // se reporta dos atributos
{
    int camion_id;
    time_t programacion;
    double ttotal;     //tiempo total de viaje de este camion
    bool TieneCamionFeeder;
    string NodeAdjacent;    //nodo origen o destino asociado a esta programación
    trioProgramacion() {}
    trioProgramacion(int ci, bool tf): camion_id(ci), programacion(static_cast<time_t>(-1)), ttotal(0), TieneCamionFeeder(tf), NodeAdjacent("-")  {}
    trioProgramacion(int ci, double tt, bool tf, string na): camion_id(ci), programacion(static_cast<time_t>(-1)), ttotal(tt), TieneCamionFeeder(tf), NodeAdjacent(na) {}
    string print_trio()
    {
        stringstream ss;
            ss << "[" << camion_id << "];" << programacion << ";";
        ss << endl;
        return ss.str();
    }        // end print_solution
    
};

bool CompareTrioTT(const trioProgramacion &T1, const trioProgramacion &T2) {
    return T1.ttotal > T2.ttotal;
}

bool CompareTrioP(const trioProgramacion &T1, const trioProgramacion &T2) {
    return T1.programacion < T2.programacion;
}

vector<trioProgramacion> sortTrioP(vector<trioProgramacion> x) {
    vector<trioProgramacion> xBajo;
    vector<trioProgramacion> xSobre;

    for (auto e = x.begin(); e != x.end(); e++) {
        if ((*e).TieneCamionFeeder) {
            xSobre.push_back(*e);
        } else {
            xBajo.push_back(*e);
        }
    }
    sort(xSobre.begin(), xSobre.end(), &CompareTrioTT);
    sort(xBajo.begin(), xBajo.end(), &CompareTrioTT);
    xSobre.insert(xSobre.end(), xBajo.begin(), xBajo.end());
    return xSobre;
}


// clase nodo
class Node
{
private:
    string codigo;         // string que me indica codigo del nodo
    int type;           //  VERSION ACTUAL 0 si es nodo ficticio, 1 SI ES PLANTA Y 2 SI ES CENTRO DE DISTRIBUCION
    double Latitud;     //latitud de nodo, hay que hacer transformación para computar distancia
    double Longitud;    //idem con longitud
    time_t ventanaA;
    time_t ventanaB;
    double tCarga;
    double tDescarga;
    int andenes;
    
public:
    Node(string cod, int typep, double latt, double longg, double vA, double vB, double tC, double tD, int nAnd): codigo(cod), type(typep), Latitud(latt), Longitud(longg), tCarga(tC), tDescarga(tD), andenes(nAnd) {
        if (vA >= 0) {
            ventanaA = static_cast<time_t>(vA * 3600 * 24);
        }
        else {
            ventanaA = vA;
        }
        if (vB >= 0) {
            ventanaB = static_cast<time_t>(vB * 3600 * 24);
        }
        else {
            ventanaB = vB;
        }
        if(((vB - vA) < 0) && (vA >=0) && (vB >= 0))
        {
            long vBT = 24*3600*(1 + vB);
            ventanaB = static_cast<time_t>(vBT);
            
        }
        
        Schedule0.clear();
        Schedule1.clear();
        Schedule2.clear();
        Schedule3.clear();
    }
    
    vector<trioProgramacion> Schedule0;  //programacion de camiones mismo día
    vector<trioProgramacion> Schedule1;  //programacion de camiones un día tarde
    vector<trioProgramacion> Schedule2;  //programacion de camiones dos días tarde
    vector<trioProgramacion> Schedule3;  //programacion de camiones tres días tarde
    
    vector<Prod_parts> demanda;     // desagregado por tipo de producto; vacío si es planta
    vector<Prod_parts> o1;          // output del modelo
    vector<Prod_parts> o2;          // output del modelo
    vector<Prod_parts> u;           // output del modelo
    
    string get_cod() {return codigo;}
    int get_type() {return type;}
    double Latit() {return Latitud;}
    double Longit() {return Longitud;}
    
    double getTcarga() {return tCarga;}
    double getTdescarga() {return tDescarga;}
    double getWidthTW() {return ventanaB - ventanaA;}
    time_t getVentanaB() {return ventanaB;}
    time_t getMiddleVentana() {return(ventanaA + 0.5*getWidthTW());}
    time_t getProgramacion(double delta) {return(ventanaB - floor(delta));}    //RAUL REVISAR UNIDADES
    
    double Dist(Node& n2);    //función que calcula distancia desde este noda hasta n2
    string print_node();
    string printProg_node();
    string printProg_nodeSYNTHESIS();
    int DiaProgramacion(time_t PR, bool* dentroTW);
    void OrdenaProgramaciones();
    int findCamionSchedule(int camion_id, int* pos);
    void Reasigna_schedule(double DeltaT, int camion_id);
    time_t TiempoCritico(vector<int>& cid);     //fn que entrega el tiempo crítico entre un grupo de camiones
    time_t CorrigeTiempoAsignacion(time_t TiempoSch);
};

double Node::Dist(Node& n2)
{
    double lat1RAD = M_PI*Latit()/180;
    double long1RAD = M_PI*Longit()/180;
    double lat2RAD = M_PI*n2.Latit()/180;
    double long2RAD = M_PI*n2.Longit()/180;
    double dlat = lat2RAD-lat1RAD;
    double dlong = long2RAD-long1RAD;
    double a = pow(sin(0.5*dlat),2)+cos(lat1RAD)*cos(lat2RAD)*pow(sin(0.5*dlong),2);
    double c = 2*asin(sqrt(a));
    
    return(1000*RAD*c);     // entrega distancia en metros
}

int Node::DiaProgramacion(time_t PR, bool* dentroTW)
{
    double HW = 0.5*getWidthTW();
    time_t VP0 = ventanaA + HW;     //mitad de la ventana en día 0
    double Dif0 = fabs(PR-VP0);
    int id_menor = -1;
    if(Dif0 < HW)
    {
        *dentroTW = true;
        return(0);  // cae en ventana de día 0
    }
    else
    {
        time_t VP1 = ventanaA + HW + 24*3600;
        double Dif1 = fabs(PR-VP1);
        if(Dif0 < Dif1)
            id_menor = 0;
        else
            id_menor = 1;
        if(Dif1 < HW)
        {
            *dentroTW = true;
            return(1);
        }
        else
        {
            time_t VP2 = ventanaA + HW + 2*24*3600;
            double Dif2 = fabs(PR-VP2);
            if(id_menor == 0)
            {
                if(Dif2 < Dif0)
                    id_menor = 2;
            }
            else if(id_menor == 1)
            {
                if(Dif2 < Dif1)
                    id_menor = 2;
            }
            
            if(Dif2 < HW)
            {
                *dentroTW = true;
                return(2);
            }
            else
            {
                time_t VP3 = ventanaA + HW + 3*24*3600;
                double Dif3 = fabs(PR-VP3);
                
                if(id_menor == 0)
                {
                    if(Dif3 < Dif0)
                        id_menor = 3;
                }
                else if(id_menor == 1)
                {
                    if(Dif3 < Dif1)
                        id_menor = 3;
                }
                else if(id_menor == 2)
                {
                    if(Dif3 < Dif2)
                        id_menor = 3;
                }
                
                if(Dif3 < HW)
                {
                    *dentroTW = true;
                    return(3);
                }
                else
                {
                    *dentroTW = false;
                    return(id_menor);
                }
            }
        }
    }
}           // end function que encuentra el día en que ocurre esta programación

int Node::findCamionSchedule(int camion_id, int* pos)
{
    for(unsigned int k=0; k<Schedule0.size(); k++)
    {
        if(camion_id == Schedule0[k].camion_id)
        {
            *pos = k;
            return(0);
        }
    }
    for(unsigned int k=0; k<Schedule1.size(); k++)
    {
        if(camion_id == Schedule1[k].camion_id)
        {
            *pos = k;
            return(1);
        }
    }
    for(unsigned int k=0; k<Schedule2.size(); k++)
    {
        if(camion_id == Schedule2[k].camion_id)
        {
            *pos = k;
            return(2);
        }
    }
    for(unsigned int k=0; k<Schedule3.size(); k++)
    {
        if(camion_id == Schedule3[k].camion_id)
        {
            *pos = k;
            return(3);
        }
    }
    *pos = -1;
    return(-1);
}

void Node::Reasigna_schedule(double DeltaT, int camion_id)
{
    bool dentroV;
    int pos;
    int CID = findCamionSchedule(camion_id,&pos);
    if(CID == 0)
    {
        Schedule0[pos].programacion -= DeltaT;
    }
    else if (CID == 1)
    {
        Schedule1[pos].programacion -= DeltaT;
        int DIA = DiaProgramacion(Schedule1[pos].programacion, &dentroV);
        if(DIA == 0)
        {
            Schedule0.push_back(Schedule1[pos]);
            Schedule1.erase(Schedule1.begin()+pos);
        }
    }
    else if (CID == 2)
    {
        Schedule2[pos].programacion -= DeltaT;
        int DIA = DiaProgramacion(Schedule2[pos].programacion, &dentroV);
        if(DIA == 0)
        {
            Schedule0.push_back(Schedule2[pos]);
            Schedule2.erase(Schedule2.begin()+pos);
        }
        else if(DIA == 1)
        {
            Schedule1.push_back(Schedule2[pos]);
            Schedule2.erase(Schedule2.begin()+pos);
        }
    }
    else if (CID == 3)
    {
        Schedule3[pos].programacion -= DeltaT;
        int DIA = DiaProgramacion(Schedule3[pos].programacion, &dentroV);
        if(DIA == 0)
        {
            Schedule0.push_back(Schedule3[pos]);
            Schedule3.erase(Schedule3.begin()+pos);
        }
        else if(DIA == 1)
        {
            Schedule1.push_back(Schedule3[pos]);
            Schedule3.erase(Schedule3.begin()+pos);
        }
        else if(DIA == 2)
        {
            Schedule2.push_back(Schedule3[pos]);
            Schedule3.erase(Schedule3.begin()+pos);
        }
        
    }   // end else if
    
}   // end function Reasigna schedule

time_t Node::TiempoCritico(vector<int>& cid)
{
    time_t ProgCritica0 = 0;
    for(unsigned int k=0; k<cid.size(); k++)
    {
        int pos;
        time_t ProgCritica;
        int sch = findCamionSchedule(cid[k], &pos);
        if(sch == 0)
            ProgCritica = Schedule0[pos].programacion;
        else if(sch == 1)
            ProgCritica = Schedule1[pos].programacion;
        else if(sch == 2)
            ProgCritica = Schedule2[pos].programacion;
        else if(sch == 3)
            ProgCritica = Schedule3[pos].programacion;
        else
            ProgCritica = 0;
        
        if(ProgCritica > ProgCritica0)
            ProgCritica0 = ProgCritica;
    }
    return(ProgCritica0);
}

time_t Node::CorrigeTiempoAsignacion(time_t TiempoSch)
{
    int deltaT = TiempoSch % 900;
    if((deltaT == 0) || (TiempoSch == -1))
        return(TiempoSch);
    else
    {
        time_t Tcorr = TiempoSch - deltaT + 900;
        if(Tcorr > ventanaB)
        {
            int delta2 = ventanaB % 900;
            if(delta2 == 0)
                return(ventanaB);
            else
            {
                time_t Tcorr2 = ventanaB - delta2 + 900;
                return (Tcorr2);
            }
        }
        else
            return(Tcorr);
    }
}       // end fn que deja cada 15 mins los schedules

void Node::OrdenaProgramaciones()
{
    sort(Schedule0.begin(),Schedule0.end(), &CompareTrioP);
    sort(Schedule1.begin(),Schedule1.end(), &CompareTrioP);
    sort(Schedule2.begin(),Schedule2.end(), &CompareTrioP);
    sort(Schedule3.begin(),Schedule3.end(), &CompareTrioP);
}

string Node::print_node()
{
    stringstream s;
    double DeltaVentana = ventanaB - ventanaA;
    s   << "(" << codigo << ";" << type << ";" << Latitud << ";" << Longitud << ";" << ventanaA << ";" << ventanaB << ";" << DeltaVentana << ";" << tCarga << ";" << tDescarga << ")";
        return s.str();
}
                                                 
string Node::printProg_node()
{
    stringstream s;
    double DeltaVentana = ventanaB - ventanaA;
    s   << "(" << codigo << ";" << type << ";" << Latitud << ";" << Longitud << ";" << ventanaA << ";" << ventanaB << ";" << DeltaVentana << ";" << tCarga << ";" << tDescarga << ")" << endl;
        for(unsigned int k=0; k<Schedule0.size(); k++)
            s << "Sch0:" << Schedule0[k].camion_id << ";" << Schedule0[k].programacion << ";[" << Schedule0[k].TieneCamionFeeder << ":" << Schedule0[k].ttotal << "];";
        s << endl;
        for(unsigned int k=0; k<Schedule1.size(); k++)
            s << "Sch1:" << Schedule1[k].camion_id << ";" << Schedule1[k].programacion << ";[" << Schedule1[k].TieneCamionFeeder << ":" << Schedule1[k].ttotal << "];";
        s << endl;
        for(unsigned int k=0; k<Schedule2.size(); k++)
            s << "Sch2:" << Schedule2[k].camion_id << ";" << Schedule2[k].programacion << ";[" << Schedule2[k].TieneCamionFeeder << ":" << Schedule2[k].ttotal << "];";
        s << endl;
        for(unsigned int k=0; k<Schedule3.size(); k++)
            s << "Sch3:" << Schedule3[k].camion_id << ";" << Schedule3[k].programacion << ";[" << Schedule3[k].TieneCamionFeeder << ":" << Schedule3[k].ttotal << "];";
        s << endl;
    
    return s.str();
}

string Node::printProg_nodeSYNTHESIS()
{
    stringstream s;
    s   << codigo << ";" << type << ";" << ventanaA << ";" << ventanaB << ";" << tCarga << ";" << tDescarga << ";" << endl;
    s << "Sch0;";
        for(unsigned int k=0; k<Schedule0.size(); k++)
           s  << Schedule0[k].camion_id << ";" << Schedule0[k].programacion << ";" << Schedule0[k].NodeAdjacent << ";";
        s << endl;
    s << "Sch1;";
        for(unsigned int k=0; k<Schedule1.size(); k++)
           s  << Schedule1[k].camion_id << ";" << Schedule1[k].programacion << ";" << Schedule1[k].NodeAdjacent << ";";
        s << endl;
    s << "Sch2;";
        for(unsigned int k=0; k<Schedule2.size(); k++)
            s  << Schedule2[k].camion_id << ";" << Schedule2[k].programacion << ";" << Schedule2[k].NodeAdjacent << ";";
        s << endl;
    s << "Sch3;";
        for(unsigned int k=0; k<Schedule3.size(); k++)
           s << Schedule3[k].camion_id << ";" << Schedule3[k].programacion << ";" << Schedule3[k].NodeAdjacent << ";";
        s << endl;
    
    return s.str();
}


struct Edge_parts
{
    string type;        // puede ser x, x2,p,z, z2, zc, z2c y, y2, xr, zr, pr, zrc, qx, qx2, qz, qz2, qzc, qz2c, ahora tb puede ser xs, zs o ys
    string atributo;    // si es x tipo de producto segun codigo, si es p o z atributo indica la familia, si es y representa cap camion en pallets
    double amount;      // para x es double para el resto es entero
    int frescura;       // se lee nuevo atributo de frescura para el caso de los x, para el resto frescura queda por defecto en -1
    bool SuperMercado;  // true si nodo es tratado como supermercado, false si no (se hace true solo si es una x)
    double pesoCaja;    // -1 para z,y, y vale el peso por caja de este producto
    int id_camionD;     // guarda el id del camión, en caso de tratarse de la asignación del modelo desagregado, vale para x y para z solamente, default value -1
    int id_predec;      // si se trata de un link x2, y2, z2, zs, ys indica id del arco que lo precede, -1 en cualquier otro caso, -2 si se identifica predecesor
    int id_sucesor;     // si se trata de un link x2, y2, z2, zs, ys indica id del arco sucesor, -1 en cualquier otro caso, -2 si se identifica sucesor
    int id_ep;          // si hay un predecesor o un sucesor, indica en el arco correspondiente, cual es el id del edge_parts que le corresponde, -1 en cualquier otro caso
    int node_id;  // nodo asociado a arco que complementa, que puede ser predecesor o sucesor (id asociado a vector Nodos)
    int descarga;   //sólo aplica en xs y zs, y vale 1 si se descarga al final del presente arco, 2 si sigue hasta el siguiente nodo sucursal (arco aguas abajo)
    double CajasPorPallet;      //cajas por pallet, ++++++++++++ RAUL AGREGAR CRITERIO POR ESTE ITEM
    
    Edge_parts(string t, string at, double am, int fr, bool sp, double pc, int ic, int idp, int ids, int nid, int des, double cjp): type(t), atributo(at), amount(am), frescura(fr), SuperMercado(sp), pesoCaja(pc), id_camionD(ic), id_predec(idp), id_sucesor(ids), id_ep(-1), node_id(nid), descarga(des), CajasPorPallet(cjp) {}
    
    void asignaPesoCaja(double pc) {pesoCaja = pc;}
    
    string print_parts()
    {
        stringstream s;
        s << "(" << type << ";" << atributo << ";" << amount << ";" << frescura << ";" << SuperMercado << ";" << pesoCaja << ";" << CajasPorPallet << ";" << id_camionD << ";" << id_predec << ";" << id_sucesor << ";" << id_ep << ";" << descarga << ")";
        return s.str();
    }
    
    bool operator< (const Edge_parts &other) const {
		bool rtn = amount > other.amount;
        if(pesoCaja > UmbralPesoCaja)
            rtn = pesoCaja > other.pesoCaja || (pesoCaja == other.pesoCaja && amount > other.amount);
		return rtn;
    }
};

bool CompareEdgeParts(const Edge_parts &E1, const Edge_parts &E2)
{
    if((E1.type == E2.type) && (E1.atributo == E2.atributo) && (E1.amount == E2.amount) && (E1.frescura == E2.frescura) && (E1.SuperMercado == E2.SuperMercado) && (E1.pesoCaja == E2.pesoCaja) && (E1.id_camionD == E2.id_camionD))
        return true;    // true si el contenido es el mismo
    else
        return false;
}

struct InfoCapac
{
    int camion_id;
    string stype;   //12P; 26P
    string TipoViaje;   //primario, interplanta
    string nodoOperacion;   //si es carga es el origen, si es descarga es el destino
    string tipoOperacion;   // carga si es en nodo orgien, descarga si en nodo destino
    string estado;
    string sector;
    bool separador;     //true si hay separador, false si no
    bool compartido;    //true si es un viaje compartido P-S-S
    double npallets;    //npallets asociados a este ítem
    double nkilos;      //idem pero con peso en kilos
    
    InfoCapac(int idc, string st, string tiv, string no, string to, string es, string sec, bool se, bool com, double np, double nk): camion_id(idc), stype(st), TipoViaje(tiv), nodoOperacion(no), tipoOperacion(to), estado(es), sector(sec), separador(se), compartido(com), npallets(np), nkilos(nk) {}
    
    string print_Capac()
    {
        stringstream s;
        s << camion_id << ";" << stype << ";" << TipoViaje << ";" << nodoOperacion << ";" << tipoOperacion << ";" << estado << ";" << sector << ";" << separador << ";" << compartido << ";" << npallets << ";" << nkilos << ";";
        return s.str();
    }
    
};

// clase edge
class Edge
{
private:
    int edge_id;    // id de edge en el vector de edges
    int unode;      // id nodo origen
    int dnode;      // ide nodo destino
    map<string, double> costT;
    map<string, Transporte> transportes;
    double tViaje;
    
public:
    Edge(int e, int u, int d): edge_id(e), unode(u), dnode(d) {}
    
    // variables para tramos directos
    vector<Edge_parts> x;
    vector<Edge_parts> p;
    vector<Edge_parts> z;
    vector<Edge_parts> y;
    
    //variables para tramos en dos capas (se repite esta información an arcos predecesores o antecesores, dependiendo el caso
    vector<Edge_parts> x2;
    vector<Edge_parts> p2;
    vector<Edge_parts> z2;
    vector<Edge_parts> y2;
    
    //variables qie se agregan para viajes compartidos
    vector<Edge_parts> xs;
    vector<Edge_parts> zs;
    vector<Edge_parts> ys;
    
    vector<Edge_parts> zc;
    vector<Edge_parts> z2c;
    
    // para consolidado
    vector<Edge_parts> xr;
    vector<Edge_parts> pr;
    vector<Edge_parts> zr;
    
    vector<Edge_parts> zrc;
    
    vector<vector<Pallet>> CargaD; //identifica los pallets familias 0,1,2,3,4... directos, como se cargan de producto y en que camión se mueven en este arco
    vector<vector<vector<Pallet>>> Carga2; //identifica los pallets familias 0,1,2,3,4... que se mueven en dos arcos, como se cargan de producto y en que camión se mueven en este arco. Además, se identifica el origen (o destino) del siguiente arco (nodo), por eso la dimensión adicional, con # el número de arcos conectados aguas arriba (o abajo) de este arco.
    vector<vector<vector<Pallet>>> CargaR; //identifica los pallets familias 0,1,2,3,4... que se mueven en dos arcos pero son de consolidación, como se cargan de producto y en que camión se mueven en este arco. Explicación de dimesnion adicional, idem al de arriba
    vector<vector<vector<Pallet>>> CargaS;  //idem, pero para viajes combinados, que ahora son P-S-S, CargaS[destinos(or)][familias][id_pallets]
    
    vector<Camion> CamionesD;    //camiones que sirven este par O-D en forma directa
    vector<vector<Camion>> Camiones2; //camiones definidos para viajes en dos tramos (P-P-S)
    vector<vector<Camion>> CamionesS; //camiones definidos para viajes en dos tramos (P-S-S)
    
    void setTViaje(double tv) { tViaje = tv; }
    double getTViaje() {return tViaje;}
    int v() { return unode;}
    int w() { return dnode;}
    double getCostT(string key) {return costT[key];}
    void setCostT(string key, double c) {costT[key] = c;}
    void setTransportes(map<string, Transporte> trans) { transportes = trans; }
    bool ArcUsed()
    { if ((x.size() == 0) && (x2.size() == 0) && (xr.size() == 0) && (xs.size() == 0))
        return(false);
      else
          return(true);
        }
  // FUNCIONES PARA CALCULAR INDICADORES AGREGADOS
    int fleet_size();
    int CapKilosTrucks();   //computa la capacidad de los camiones que más se repiten en este arco
    double sumUsoX(vector<Edge_parts>& xP, vector<Producto>& Prod);     //uso de kilos totales
    double sumUsoP(vector<Edge_parts>& xP, vector<Producto>& Prod);     //uso de pallets totales
    double sumYKilos(vector<Edge_parts>& yP);                         // suma de cap de kilos de camiones
    double sumYPallets(vector<Edge_parts>& yP);                       // suma de cap en pallets
    
    // En estas funciones Indicador vale 1 si proceso Camiones y CamionesD, vale 2 si proceso sólo CamionesD y vale 3 si proceso solo Camiones2
    double sumUsoX_camiones(vector<Producto>& Prod, string ProdCode, int Indicador);     // obtenidos de la carga de pallets sobre camiones en ese arco
    double sumCajasX_camiones(vector<Producto>& Prod, string ProdCode, int Indicador, bool super);             // entrega la cantidad total de cajas que se mueven en el arco
    double sumCajasX_camionesTodo(vector<Producto>& Prod, string ProdCode, int Indicador);
    double sumPallets_camiones(vector<Producto>& Prod, int Indicador);   //cuenta el número total de pallets usados, incluyendo pallet virtual requerido para emprejar congelados
    double sumUsoP_camiones(vector<Producto>& Prod, string ProdCode, int Indicador);     //uso de pallets totales
    double sumYKilos_camiones(int Indicador);                         // suma de cap de kilos de camiones
    double sumYPallets_camiones(int Indicador);                       // suma de cap en pallets
    
    double sumCosto(vector<Producto>& Prod, vector<Node>& Nodes, int Indicador);             // costos de distribución asociados a costo variable por arco.
    
    //FUNCIONES QUE ACTUALIZAN INFORMACIÓN DE PALLETS EN ARCO PREDECESOR O SUCESOR
    bool searchPallet(int idPallet, bool* r, int* i, int* j, int* k);  // busca el pallet en el arco, ya sea de consolidación (en tal caso r = true) o de paso entre dos arcos (en tal caso r = false). Los outputs i, j entregan los íncdices de ya sea Carga2 o CargaR, según bool r. Return true si la encuentra, false en otro caso
    bool searchPalletS(int idPallet, int* i, int* j, int* k);
    bool searchPalletCamion(int idPallet, int* i, int* j, int* id_vpallets); //mismo de arriba pero busca en Camiones2 (i,j), y entrega además el id en vextor de pallets dentro de Camion, return false si no lo encuentra
    bool searchPalletCamionS(int idPallet, int* i, int* j, int* id_vpallets);
    bool searchCamion(int idCamion, int* i, int* j);    //entrega posición en Camiones2, si lo encuentra
    bool searchCamionS(int idCamion, int* i, int* j);
    int searchCamionTotal(int idCamion, int* i, int* j);  //idem pero busca en Camiones y Camiones2
    void findDestinos(vector<Edge_parts>& xP, set<int>& destinos);
    void findOrigenes(vector<Camion>& cP, set<int>& origenes);
    //FUNCIONES PARA CARGAR PRODUCTOS A PALLETS Y PALLETS EN CAMIONES
    void updateCarga(vector<vector<Pallet>>& CargaP, vector<Producto>& Prod, int fam);         // se hace un update de las occupancies y se ordenan los vectores de carga segun ocup
    void asignaPesoCajaX(vector<Edge_parts>& xP, vector<Producto>& Prod);
    void consolida_carga(vector<vector<vector<Pallet>>>& CargaP, vector<vector<Pallet>>& CargaPCons, size_t NF);
    void consolida_camiones(vector<vector<Camion>>& CamionesP, vector<Camion>& CamionesPCons);
    void desconsolida_camiones(vector<vector<Camion>>& CamionesP, vector<Camion>& CamionesPCons);
    void separa_camiones(vector<Camion>& CamionesPAgregado, vector<Camion>& CamionesP, vector<Camion>& CamionesPCons);
    int createPallets(int idFirstPallet, int filtro_origen, int filtro_destino, int filtro_descarga, vector<Edge_parts>& xP, vector<Edge_parts>& zP, vector<vector<Pallet>>& CargaP, vector<Producto>& Prod, vector<Familia>& FA);       // crea los pallets de acuerdo con vector z
    void ProdPallet(double Amount, int i, string type, vector<Pallet>& CargaP, Producto Prod, vector<Edge_parts>& Sobrante, int frescura, bool super, int id_pred, int id_suc, int id_ep, int node_id, int des);  // fn con algoritmo que carga x de una familia en pallets disponibles, i es el próximo pallet a cargar
    void CargaAmountEP(Edge_parts xPc, vector<Producto>& Prod, vector<vector<Pallet>>& CargaP, vector<Edge_parts>& Sobrante);
    int cargaPalletsProductos(int idFirstPallet, vector<Producto>& Prod, vector<Edge_parts>& xP, vector<Edge_parts>& zP, vector<vector<Pallet>>& CargaP, int filtro_origen, int filtro_destino, int filtro_descarga, vector<Edge_parts>& Sobrante, vector<Familia>& FA);    // carga los pallets a los productos
    int createCamiones(int idFirstCamion, int filtro_origen, int filtro_destino, vector<Edge_parts>& yP, vector<Camion>& CamionesP, vector<Node>& Nodes, size_t NF);
    void LoadCamiones(vector<Camion>& CamionesP, int k, vector<Pallet>& P1, int p1_id, vector<Pallet>& P2, int p2_id, vector<Producto>& Prod, vector<trioPallet>& sobranteP);
    void LoadCamionesSingle(vector<Camion>& CamionesP, int k, vector<Pallet>& P1, int p1_id, vector<Producto>& Prod, vector<trioPallet>& sobranteP, vector<Familia>& FA);
    void Carga_CongeladoV0(vector<Camion>& CamionesP, vector<Pallet>& CargaCongelados, bool HayFresco, size_t SizeCarga2, vector<trioPallet>& sobranteC, vector<Producto>& Prod);
    void Carga_CongeladoV1(vector<Camion>& CamionesP, vector<Pallet>& CargaCongelados, bool HayFresco, size_t SizeCarga2, vector<trioPallet>& sobranteC, vector<Producto>& Prod);
    void Carga_CongeladoV2(vector<Camion>& CamionesP, vector<Pallet>& CargaCongelados, bool HayFresco, size_t SizeCarga2, vector<trioPallet>& sobranteC, vector<Producto>& Prod);
    int LoadCamionesEdge(int idFirstCamion, vector<Camion>& CamionesPrevios, vector<Camion>& CamionesP, vector<vector<Pallet>>& CargaP1, vector<vector<Pallet>>& CargaP2, vector<Edge_parts>& yP, vector<Producto>& Prod, vector<Node>& Nodes, int filtro_origen, int filtro_destino, vector<Pallet>& sobranteF,bool consolidacion, vector<Familia>& FA);
    bool CheckPalletCargado(int search_idP);
    
    //funciones para cargar pallets no cargados, en camiones que estoy analizando
    void FixPalletsNoCargadosPP(vector<Camion>& CamAux, vector<Pallet>& PNC, int noptions, vector<Producto>& Prod, vector<Pallet>& sobP, vector<Familia>& FA);
    void FixPalletsNoCargadosPS(vector<Camion>& CamAux, vector<Camion>& CamCons, vector<Pallet>& PNC, int noptions, vector<Producto>& Prod, vector<Pallet>& sobP, vector<Familia>& FA);
    Pallet RemovePallet(int idCam, vector<Camion>& CamAux, bool congelado, vector<Familia>& FA);
  // fns que ajustan camiones carga congelados
    bool MovePalletC(int idCamO, int idCamD, vector<Camion>& CamAux, vector<Producto>& Prod);
    bool AjustaCamiones(vector<Camion>& CamAux, vector<Producto>& Prod);
    bool AjustaCamiones2(vector<Camion>& CamAux, vector<Producto>& Prod);   //caso de cargar camiones a tope
    void AjustaTV(vector<Node>& Nodes);
    
    //función que entrega estadística de carga en este nodo, por sector-estado
    vector<InfoCapac> DetalleAnalisisCapacidad(bool carga, vector<Node>& Nodes, vector<Producto>& Prod, vector<Familia>& FA);
    
  // IMPRIMIENDO SÍNTESIS DE RESULTADOS
    
    double Pallets_producto(string id_producto, vector<Node>& Nodes, vector<Producto>& Prod); //cuenta número de pallets de un cierto producto id_producto llegando a nodo fin de EdgeIN
    double Cajas_producto(string id_producto, vector<Node>& Nodes, vector<Producto>& Prod);    //idem pero con número de cajas
    string PalletsNoCargados(vector<Node>& Nodes, vector<Producto>& Prod);
    string PrintDetails(string IDEN, vector<Camion>& CamionesP, vector<vector<Pallet>>& CargaP, vector<Node>& Nodes, vector<Producto>& Prod);
    string PrintDetailsD(string IDEN, vector<vector<Camion>>& CamionesP, vector<vector<vector<Pallet>>>& CargaP, vector<Node>& Nodes, vector<Producto>& Prod);
    string PrintDetailsD2(string IDEN, vector<Camion>& CamionesP, vector<vector<vector<Pallet>>>& CargaP, vector<Node>& Nodes, vector<Producto>& Prod);
    string PrintDetails2(vector<Node>& Nodes, vector<Producto>& Prod);
    string PrintCarga(vector<Node>& Nodes, vector<Producto>& Prod);
    string PrintCamiones(vector<Node>& Nodes, vector<Producto>& Prod);
    string PrintCamionesDetails(vector<Node>& Nodes, vector<Producto>& Prod);
    string PrintTVHaversine(vector<Node>& Nodes);
};

int get_Edge_id(string N0, string N1, vector<Edge>& Edges, vector<Node>& Nodes)
{
    for(unsigned int k=0; k<Edges.size(); k++)
    {
        if((Nodes[Edges[k].v()].get_cod() == N0)&&(Nodes[Edges[k].w()].get_cod() == N1))
            return(k);
    }
    return(-1);     //no existe arco con esas características
}

int Edge::CapKilosTrucks()      //computa el peso de los camiones que más se repiten en ese arco
{
    map<string, int> kk;
    string keyMax = "";
    int valMax = 0;

    for(unsigned int i=0; i<y.size(); i++)
    {
        if(kk.find(y[i].atributo) == kk.end())
            kk[y[i].atributo]=0;
        kk[y[i].atributo]++;
    }
    
    for(unsigned int i=0; i<y2.size(); i++)
    {
        if(kk.find(y2[i].atributo) == kk.end())
            kk[y2[i].atributo]=0;
        kk[y2[i].atributo]++;
    }
    
    for(unsigned int i=0; i<ys.size(); i++)
    {
        if(kk.find(ys[i].atributo) == kk.end())
            kk[ys[i].atributo]=0;
        kk[ys[i].atributo]++;
    }

    for (auto it = kk.begin(); it != kk.end(); it++)
    {
 
        if (it->second > valMax) {
            valMax = it->second;
            keyMax = it->first;
        }
    
        else if (it->second == valMax && transportes[it->first].peso > transportes[keyMax].peso) {
                    valMax = it->second;
                    keyMax = it->first;
                }
    }

    return(transportes[keyMax].peso);
}

int Edge::fleet_size()
{
    int sum = 0;
    if(Camiones2.size() > 0)
    {
        for(unsigned int r=0; r<Camiones2.size(); r++)
        {
            if(Camiones2[r].size() > 0)
            {
                for(unsigned int m=0; m<Camiones2[r].size(); m++)
                {
                    if(Camiones2[r][m].carga.size() > 0)
                        sum++;
                }
            }
        }
    }       // end if
    
    if(CamionesD.size() > 0)
    {
        for(unsigned int r=0; r<CamionesD.size(); r++)
        {
            if(CamionesD[r].carga.size() > 0)
                sum++;
        }
    }       // end if
    
    if(CamionesS.size() > 0)
    {
        for(unsigned int r=0; r<CamionesS.size(); r++)
        {
            if(CamionesS[r].size() > 0)
            {
                for(unsigned int m=0; m<CamionesS[r].size(); m++)
                {
                    if(CamionesS[r][m].carga.size() > 0)
                        sum++;
                }
            }
        }
    }       // end if
    
    return(sum);
}

double Edge::sumUsoX(vector<Edge_parts>& xP, vector<Producto>& Prod)
{
    double sum = 0;
    if(xP.size() > 0)
    {
        for(unsigned int r=0; r<xP.size(); r++)
        {
            int Prod_id = get_Prod_id(xP[r].atributo,Prod);
            double Peso = Prod[Prod_id].get_PesoCaja();
            sum += xP[r].amount*Peso;
        }
    }       // end if
    return(sum);
}      // end sum sobre todo producto

// para cada arco suma el uso de pallets
double Edge::sumUsoP(vector<Edge_parts>& xP, vector<Producto>& Prod)
{
    double sum = 0;
    if(xP.size() > 0)
    {
        for(unsigned int r=0; r<xP.size(); r++)
        {
            int Prod_id = get_Prod_id(xP[r].atributo,Prod);
            double CajasP = Prod[Prod_id].get_CajasPallet();
            sum += (double)xP[r].amount/(double)CajasP;
        }
    }       // end if
    return(sum);
}      // end sum sobre todo producto

double Edge::sumYKilos(vector<Edge_parts>& yP)
{
    double sum = 0;
    if(yP.size() > 0)
    {
        for(unsigned int r=0; r<yP.size(); r++)
            sum += yP[r].amount*transportes[yP[r].atributo].peso;
    }       // end if
    return(sum);
}      // end sum sobre todo producto

double Edge::sumYPallets(vector<Edge_parts>& yP)
{
    double sum = 0;
    if(yP.size() > 0)
    {
        for(unsigned int r=0; r<yP.size(); r++)
        {
            /*if(yP[r].atributo == "Truck2")
             sum += yP[r].amount*CapPalletsT2;
            else if(yP[r].atributo == "Truck1")
             sum += yP[r].amount*CapPalletsT1;*/
            sum += yP[r].amount*transportes[yP[r].atributo].volumen;
        }
    }       // end if
    return(sum);
}      // end sum sobre todo producto

// funciones que calculan lo mismo anterior, pero a partir de la asignación de cajas a pallets y pallets a camciones, tanto CamionesD como Camiones2
double Edge::sumUsoX_camiones(vector<Producto>& Prod, string ProdCode, int Indicador)     // obtenidos de la carga de pallets sobre camiones en ese arco, si ProdCode == "-1" significa que procesa todos los códigos. Indicador vale 1 si proceso Camiones2, CamionesS y CamionesD, vale 2 si proceso sólo CamionesD y vale 3 si proceso solo Camiones2 y CamionesS
{
    double sum = 0;
    if((Camiones2.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<Camiones2.size(); r++)
        {
            if(Camiones2[r].size() > 0)
            {
                for(unsigned int m=0; m<Camiones2[r].size(); m++)
                {
                    if(Camiones2[r][m].carga.size() > 0)
                    {
                        for(unsigned int p=0; p<Camiones2[r][m].carga.size(); p++)
                        {
                            Camiones2[r][m].carga[p].OcupPesoCuad(Prod);
                            if(ProdCode == "-1")
                                sum += Camiones2[r][m].carga[p].peso;
                            else
                            {
                                if(Camiones2[r][m].carga[p].BC.size() > 0)
                                {
                                    for(unsigned int z=0; z<Camiones2[r][m].carga[p].BC.size(); z++)
                                    {
                                        if(Camiones2[r][m].carga[p].BC[z].getProduct(Prod) == ProdCode)
                                            sum += Camiones2[r][m].carga[p].BC[z].Peso_bunch(Prod);
                                    }
                                }
                                
                            }
                            
                        }
                    }
                }
            }
           
        }
    }       // end if
    
    if((CamionesS.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<CamionesS.size(); r++)
        {
            if(CamionesS[r].size() > 0)
            {
                for(unsigned int m=0; m<CamionesS[r].size(); m++)
                {
                    if(CamionesS[r][m].carga.size() > 0)
                    {
                        for(unsigned int p=0; p<CamionesS[r][m].carga.size(); p++)
                        {
                            CamionesS[r][m].carga[p].OcupPesoCuad(Prod);
                            if(ProdCode == "-1")
                                sum += CamionesS[r][m].carga[p].peso;
                            else
                            {
                                if(CamionesS[r][m].carga[p].BC.size() > 0)
                                {
                                    for(unsigned int z=0; z<CamionesS[r][m].carga[p].BC.size(); z++)
                                    {
                                        if(CamionesS[r][m].carga[p].BC[z].getProduct(Prod) == ProdCode)
                                            sum += CamionesS[r][m].carga[p].BC[z].Peso_bunch(Prod);
                                    }
                                }
                                
                            }
                            
                        }
                    }
                }
            }
           
        }
    }       // end if
    
    if((CamionesD.size() > 0)&&(Indicador != 3))
    {
        for(unsigned int r=0; r<CamionesD.size(); r++)
        {
            if(CamionesD[r].carga.size() > 0)
            {
                for(unsigned int p=0; p<CamionesD[r].carga.size(); p++)
                {
                    CamionesD[r].carga[p].OcupPesoCuad(Prod);
                    if(ProdCode == "-1")
                        sum += CamionesD[r].carga[p].peso;
                    else
                    {
                        if(CamionesD[r].carga[p].BC.size() > 0)
                        {
                            for(unsigned int z=0; z<CamionesD[r].carga[p].BC.size(); z++)
                            {
                                if(CamionesD[r].carga[p].BC[z].getProduct(Prod) == ProdCode)
                                    sum += CamionesD[r].carga[p].BC[z].Peso_bunch(Prod);
                            }
                        }
                        
                    }
                }
            }
        }
    }       // end if
    
    return(sum);
}       // end de cómputo de peso total de pallets


double Edge::sumCajasX_camiones(vector<Producto>& Prod, string ProdCode, int Indicador, bool super)     // computa las cajas obtenidos de la carga de pallets sobre camiones en ese arco, idem al anterior para ProdCode o no (string -1). Indicador vale 1 si proceso Camiones2, CamionesS y CamionesD, vale 2 si proceso sólo CamionesD y vale 3 si proceso solo Camiones2 y CamionesS
{
    double sum = 0;
    if((Camiones2.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<Camiones2.size(); r++)
        {
            if(Camiones2[r].size() > 0)
            {
                for(unsigned int m=0; m<Camiones2[r].size(); m++)
                {
                    if(Camiones2[r][m].carga.size() > 0)
                    {
                        for(unsigned int p=0; p<Camiones2[r][m].carga.size(); p++)
                        {
                            if(ProdCode == "-1")
                                sum += Camiones2[r][m].carga[p].nCajas();
                            else
                            {
                                if(Camiones2[r][m].carga[p].BC.size() > 0)
                                {
                                    for(unsigned int z=0; z<Camiones2[r][m].carga[p].BC.size(); z++)
                                    {
                                        if(Camiones2[r][m].carga[p].BC[z].getProduct(Prod) == ProdCode)
                                        {
                                            if(super == Camiones2[r][m].carga[p].BC[z].getSuper())
                                               sum += Camiones2[r][m].carga[p].BC[z].getCajas();
                                        }
                                    }
                                }
                                
                            }
                            
                        }
                    }

                }
            }
           
        }
    }       // end if
    
    if((CamionesS.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<CamionesS.size(); r++)
        {
            if(CamionesS[r].size() > 0)
            {
                for(unsigned int m=0; m<CamionesS[r].size(); m++)
                {
                    if(CamionesS[r][m].carga.size() > 0)
                    {
                        for(unsigned int p=0; p<CamionesS[r][m].carga.size(); p++)
                        {
                            if(ProdCode == "-1")
                                sum += CamionesS[r][m].carga[p].nCajas();
                            else
                            {
                                if(CamionesS[r][m].carga[p].BC.size() > 0)
                                {
                                    for(unsigned int z=0; z<CamionesS[r][m].carga[p].BC.size(); z++)
                                    {
                                        if(CamionesS[r][m].carga[p].BC[z].getProduct(Prod) == ProdCode)
                                        {
                                            if(super == CamionesS[r][m].carga[p].BC[z].getSuper())
                                               sum += CamionesS[r][m].carga[p].BC[z].getCajas();
                                        }
                                    }
                                }
                                
                            }
                            
                        }
                    }

                }
            }
           
        }
    }       // end if
    
    if((CamionesD.size() > 0)&&(Indicador != 3))
    {
        for(unsigned int r=0; r<CamionesD.size(); r++)
        {
            if(CamionesD[r].carga.size() > 0)
            {
                for(unsigned int p=0; p<CamionesD[r].carga.size(); p++)
                {
                    CamionesD[r].carga[p].OcupPesoCuad(Prod);
                    if(ProdCode == "-1")
                        sum += CamionesD[r].carga[p].nCajas();
                    else
                    {
                        if(CamionesD[r].carga[p].BC.size() > 0)
                        {
                            for(unsigned int z=0; z<CamionesD[r].carga[p].BC.size(); z++)
                            {
                                if(CamionesD[r].carga[p].BC[z].getProduct(Prod) == ProdCode)
                                {
                                    if(super == CamionesD[r].carga[p].BC[z].getSuper())
                                        sum += CamionesD[r].carga[p].BC[z].getCajas();
                                }
                                    
                            }
                        }
                        
                    }
                }
            }
        }
    }       // end if
    
    return(sum);
}       // end de cómputo de número de cajas totales

double Edge::sumCajasX_camionesTodo(vector<Producto>& Prod, string ProdCode, int Indicador)     // computa las cajas obtenidos de la carga de pallets sobre camiones en ese arco, idem al anterior para ProdCode o no (string -1). Indicador vale 1 si proceso Camiones2, CamionesS y CamionesD, vale 2 si proceso sólo CamionesD y vale 3 si proceso solo Camiones2 y CamionesS
{
    double sum = 0;
    if((Camiones2.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<Camiones2.size(); r++)
        {
            if(Camiones2[r].size() > 0)
            {
                for(unsigned int m=0; m<Camiones2[r].size(); m++)
                {
                    if(Camiones2[r][m].carga.size() > 0)
                    {
                        for(unsigned int p=0; p<Camiones2[r][m].carga.size(); p++)
                        {
                            if(ProdCode == "-1")
                                sum += Camiones2[r][m].carga[p].nCajas();
                            else
                            {
                                if(Camiones2[r][m].carga[p].BC.size() > 0)
                                {
                                    for(unsigned int z=0; z<Camiones2[r][m].carga[p].BC.size(); z++)
                                    {
                                        if(Camiones2[r][m].carga[p].BC[z].getProduct(Prod) == ProdCode)
                                        {
                                               sum += Camiones2[r][m].carga[p].BC[z].getCajas();
                                        }
                                    }
                                }
                                
                            }
                            
                        }
                    }

                }
            }
           
        }
    }       // end if
    
    if((CamionesS.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<CamionesS.size(); r++)
        {
            if(CamionesS[r].size() > 0)
            {
                for(unsigned int m=0; m<CamionesS[r].size(); m++)
                {
                    if(CamionesS[r][m].carga.size() > 0)
                    {
                        for(unsigned int p=0; p<CamionesS[r][m].carga.size(); p++)
                        {
                            if(ProdCode == "-1")
                                sum += CamionesS[r][m].carga[p].nCajas();
                            else
                            {
                                if(CamionesS[r][m].carga[p].BC.size() > 0)
                                {
                                    for(unsigned int z=0; z<CamionesS[r][m].carga[p].BC.size(); z++)
                                    {
                                        if(CamionesS[r][m].carga[p].BC[z].getProduct(Prod) == ProdCode)
                                        {
                                               sum += CamionesS[r][m].carga[p].BC[z].getCajas();
                                        }
                                    }
                                }
                                
                            }
                            
                        }
                    }

                }
            }
           
        }
    }       // end if
    
    if((CamionesD.size() > 0)&&(Indicador != 3))
    {
        for(unsigned int r=0; r<CamionesD.size(); r++)
        {
            if(CamionesD[r].carga.size() > 0)
            {
                for(unsigned int p=0; p<CamionesD[r].carga.size(); p++)
                {
                    CamionesD[r].carga[p].OcupPesoCuad(Prod);
                    if(ProdCode == "-1")
                        sum += CamionesD[r].carga[p].nCajas();
                    else
                    {
                        if(CamionesD[r].carga[p].BC.size() > 0)
                        {
                            for(unsigned int z=0; z<CamionesD[r].carga[p].BC.size(); z++)
                            {
                                if(CamionesD[r].carga[p].BC[z].getProduct(Prod) == ProdCode)
                                {
                                        sum += CamionesD[r].carga[p].BC[z].getCajas();
                                }
                                    
                            }
                        }
                        
                    }
                }
            }
        }
    }       // end if
    
    return(sum);
}       // end de cómputo de número de cajas totales


double Edge::sumPallets_camiones(vector<Producto>& Prod, int Indicador)     // cantidad total de pallets transportados. Indicador vale 1 si proceso Camiones2, CamionesS y CamionesD, vale 2 si proceso sólo CamionesD y vale 3 si proceso solo Camiones2 y CamionesS
{
    double sum = 0;
    if((Camiones2.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<Camiones2.size(); r++)
        {
            if(Camiones2[r].size() > 0)
            {
                for(unsigned int m=0; m<Camiones2[r].size(); m++)
                {
                    if(Camiones2[r][m].carga.size() > 0)
                    {
                        for(unsigned int p=0; p<Camiones2[r][m].carga.size(); p++)
                        {
                            Camiones2[r][m].carga[p].OcupPesoCuad(Prod);
                            if((Camiones2[r][m].carga[p].peso > 0) || (Camiones2[r][m].carga[p].getid_pallet() == -100))
                                sum++;
                        }
                            
                    }
                }
            }
           
        }
    }       // end if
    
    if((CamionesS.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<CamionesS.size(); r++)
        {
            if(CamionesS[r].size() > 0)
            {
                for(unsigned int m=0; m<CamionesS[r].size(); m++)
                {
                    if(CamionesS[r][m].carga.size() > 0)
                    {
                        for(unsigned int p=0; p<CamionesS[r][m].carga.size(); p++)
                        {
                            CamionesS[r][m].carga[p].OcupPesoCuad(Prod);
                            if((CamionesS[r][m].carga[p].peso > 0) || (CamionesS[r][m].carga[p].getid_pallet() == -100))
                                sum++;
                        }
                            
                    }
                }
            }
           
        }
    }       // end if
    
    if((CamionesD.size() > 0)&&(Indicador != 3))
    {
        for(unsigned int r=0; r<CamionesD.size(); r++)
        {
            if(CamionesD[r].carga.size() > 0)
            {
                for(unsigned int p=0; p<CamionesD[r].carga.size(); p++)
                {
                    CamionesD[r].carga[p].OcupPesoCuad(Prod);
                    if((CamionesD[r].carga[p].peso > 0) || (CamionesD[r].carga[p].getid_pallet() == -100))
                        sum++;
                }
            }
        }
    }       // end if
    
    return(sum);
}       // end de cómputo de número de cajas totales


double Edge::sumUsoP_camiones(vector<Producto>& Prod, string ProdCode, int Indicador)     //uso de pallets totales computados de la carga de los camiones. Indicador vale 1 si proceso Camiones2, CamionesS y CamionesD, vale 2 si proceso sólo CamionesD y vale 3 si proceso solo Camiones2 y CamionesS
{
    double sum = 0;
    if((Camiones2.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<Camiones2.size(); r++)
        {
            if(Camiones2[r].size() > 0)
            {
                for(unsigned int m=0; m<Camiones2[r].size(); m++)
                {
                    if(Camiones2[r][m].carga.size() > 0)
                    {
                        for(unsigned int p=0; p<Camiones2[r][m].carga.size(); p++)
                        {
                            Camiones2[r][m].carga[p].OcupPesoCuad(Prod);
                            if(ProdCode == "-1")
                                sum += Camiones2[r][m].carga[p].ocupacion;
                            else
                            {
                                if(Camiones2[r][m].carga[p].BC.size() > 0)
                                {
                                    for(unsigned int z=0; z<Camiones2[r][m].carga[p].BC.size(); z++)
                                    {
                                        if(Camiones2[r][m].carga[p].BC[z].getProduct(Prod) == ProdCode)
                                            sum += Camiones2[r][m].carga[p].BC[z].ocupBunch(Prod);
                                    }
                                }
                                
                            }
                            
                        }
                
                    }
                }
            }
           
        }
    }       // end if
    
    if((CamionesS.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<CamionesS.size(); r++)
        {
            if(CamionesS[r].size() > 0)
            {
                for(unsigned int m=0; m<CamionesS[r].size(); m++)
                {
                    if(CamionesS[r][m].carga.size() > 0)
                    {
                        for(unsigned int p=0; p<CamionesS[r][m].carga.size(); p++)
                        {
                            CamionesS[r][m].carga[p].OcupPesoCuad(Prod);
                            if(ProdCode == "-1")
                                sum += CamionesS[r][m].carga[p].ocupacion;
                            else
                            {
                                if(CamionesS[r][m].carga[p].BC.size() > 0)
                                {
                                    for(unsigned int z=0; z<CamionesS[r][m].carga[p].BC.size(); z++)
                                    {
                                        if(CamionesS[r][m].carga[p].BC[z].getProduct(Prod) == ProdCode)
                                            sum += CamionesS[r][m].carga[p].BC[z].ocupBunch(Prod);
                                    }
                                }
                                
                            }
                            
                        }
                
                    }
                }
            }
           
        }
    }       // end if
    
    if((CamionesD.size() > 0)&&(Indicador != 3))
    {
        for(unsigned int r=0; r<CamionesD.size(); r++)
        {
            if(CamionesD[r].carga.size() > 0)
            {
                for(unsigned int p=0; p<CamionesD[r].carga.size(); p++)
                {
                    CamionesD[r].carga[p].OcupPesoCuad(Prod);
                    if(ProdCode == "-1")
                        sum += CamionesD[r].carga[p].ocupacion;
                    else
                    {
                        if(CamionesD[r].carga[p].BC.size() > 0)
                        {
                            for(unsigned int z=0; z<CamionesD[r].carga[p].BC.size(); z++)
                            {
                                if(CamionesD[r].carga[p].BC[z].getProduct(Prod) == ProdCode)
                                    sum += CamionesD[r].carga[p].BC[z].ocupBunch(Prod);
                            }
                        }
                        
                    }
                }
            }
        }
    }       // end if
    
    return(sum);
}               // end UsoPallets a partir de carga de camiones

double Edge::sumYKilos_camiones(int Indicador)                        // suma de cap de kilos de camiones. Indicador vale 1 si proceso Camiones2, CamionesS y CamionesD, vale 2 si proceso sólo CamionesD y vale 3 si proceso solo Camiones2 y CamionesS
{
    double sum = 0;
    if((Camiones2.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<Camiones2.size(); r++)
        {
            if(Camiones2[r].size() > 0)
            {
                for(unsigned int m=0; m<Camiones2[r].size(); m++)
                    sum += Camiones2[r][m].capacK;
            }
           
        }
    }       // end if
    
    if((CamionesS.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<CamionesS.size(); r++)
        {
            if(CamionesS[r].size() > 0)
            {
                for(unsigned int m=0; m<CamionesS[r].size(); m++)
                    sum += CamionesS[r][m].capacK;
            }
           
        }
    }       // end if
    
    if((CamionesD.size() > 0)&&(Indicador != 3))
    {
        for(unsigned int r=0; r<CamionesD.size(); r++)
            sum += CamionesD[r].capacK;
    }       // end if
    
    return(sum);
}

double Edge::sumYPallets_camiones(int Indicador)                       // suma de cap en pallets. Indicador vale 1 si proceso Camiones2, CamionesS y CamionesD, vale 2 si proceso sólo CamionesD y vale 3 si proceso solo Camiones2 y CamionesS
{
    double sum = 0;
    if((Camiones2.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<Camiones2.size(); r++)
        {
            if(Camiones2[r].size() > 0)
            {
                for(unsigned int m=0; m<Camiones2[r].size(); m++)
                    sum += Camiones2[r][m].capacP;
            }
           
        }
    }       // end if
    
    if((CamionesS.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<CamionesS.size(); r++)
        {
            if(CamionesS[r].size() > 0)
            {
                for(unsigned int m=0; m<CamionesS[r].size(); m++)
                    sum += CamionesS[r][m].capacP;
            }
           
        }
    }       // end if
    
    if((CamionesD.size() > 0)&&(Indicador != 3))
    {
        for(unsigned int r=0; r<CamionesD.size(); r++)
            sum += CamionesD[r].capacP;
    }       // end if
    
    return(sum);
}


//Indicador vale 1 si proceso Camiones2, CamionesS y CamionesD, vale 2 si proceso sólo CamionesD y vale 3 si proceso solo Camiones2 y CamionesS
double Edge::sumCosto(vector<Producto>& Prod, vector<Node>& Nodes, int Indicador)
{
    double sumYCV = 0;
    // CÁLCULO DE COSTOS VARIABLES
    if((Camiones2.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<Camiones2.size(); r++)
        {
            if(Camiones2[r].size() > 0)
            {
                for(unsigned int m=0; m<Camiones2[r].size(); m++)
                {
//                    outPrueba << "Arco2: " << edge_id << ";" <<  costT1 << ";" << costT2 << endl;
                        
                    /*if(Camiones2[r][m].get_stype() == "Truck1")
                        sumYCV += costT1;
                    else if(Camiones2[r][m].get_stype() == "Truck2")
                        sumYCV += costT2;*/
                    sumYCV += costT[Camiones2[r][m].get_stype()];
                }
            }       // end if
        }           // end for r
    }       // end if
    
    if((CamionesS.size() > 0)&&(Indicador != 2))
    {
        for(unsigned int r=0; r<CamionesS.size(); r++)
        {
            if(CamionesS[r].size() > 0)
            {
                for(unsigned int m=0; m<CamionesS[r].size(); m++)
                {
                    sumYCV += costT[CamionesS[r][m].get_stype()];
                }
            }       // end if
        }           // end for r
    }       // end if
        
    if((CamionesD.size() > 0)&&(Indicador != 3))
    {
        for(unsigned int r=0; r<CamionesD.size(); r++)
        {
//            outPrueba << "ArcoD: " << edge_id << ";" <<  costT1 << ";" << costT2 << endl;
                
            /*if(CamionesD[r].get_stype() == "Truck1")
                sumYCV += costT1;
            else if(CamionesD[r].get_stype() == "Truck2")
                sumYCV += costT2;*/
            sumYCV += costT[CamionesD[r].get_stype()];
        }   // end for r
    }       // end if
        

    return(sumYCV);
    
}   // end SumCosto

bool Edge::searchPallet(int idPallet, bool* r, int* i, int* j, int* k)  // busca el pallet en el arco, ya sea de consolidación (en tal caso r = true) o de paso entre dos arcos (en tal caso r = false). Los outputs i, j entregan los íncdices de ya sea Carga2 o CargaR, según bool r. Return true si la encuentra, false en otro caso
{
    for(unsigned int ii=0; ii<Carga2.size(); ii++)
    {
        for(unsigned int jj=0; jj<Carga2[ii].size(); jj++)
        {
            for(unsigned int kk=0; kk<Carga2[ii][jj].size(); kk++)
            {
                if(Carga2[ii][jj][kk].getid_pallet() == idPallet)
                {
                    *r = false;
                    *i = ii;
                    *j = jj;
                    *k = kk;
                    return(true);
                }
            }
        }
    }       // end for cerrando búsqueda
    
    for(unsigned int ii=0; ii<CargaR.size(); ii++)
    {
        for(unsigned int jj=0; jj<CargaR[ii].size(); jj++)
        {
            for(unsigned int kk=0; kk<CargaR[ii][jj].size(); kk++)
            {
                if(CargaR[ii][jj][kk].getid_pallet() == idPallet)
                {
                    *r = true;
                    *i = ii;
                    *j = jj;
                    *k = kk;
                    return(true);
                }
            }
        }
    }       // end for cerrando búsqueda
    
    *r = NULL;
    *i = NULL;
    *j = NULL;
    *k = NULL;
    return(false);
    
}   // end searchPallet

bool Edge::searchPalletS(int idPallet, int* i, int* j, int* k)  // busca el pallet en el arco, en CargaS (viajes combinados exclusivamente). Los outputs i, j entregan los íncdices de CargaS o. Return true si la encuentra, false en otro caso
{
    for(unsigned int ii=0; ii<CargaS.size(); ii++)
    {
        for(unsigned int jj=0; jj<CargaS[ii].size(); jj++)
        {
            for(unsigned int kk=0; kk<CargaS[ii][jj].size(); kk++)
            {
                if(CargaS[ii][jj][kk].getid_pallet() == idPallet)
                {
                    *i = ii;
                    *j = jj;
                    *k = kk;
                    return(true);
                }
            }
        }
    }       // end for cerrando búsqueda
    
    *i = NULL;
    *j = NULL;
    *k = NULL;
    return(false);
    
}   // end searchPallet

bool Edge::searchPalletCamion(int idPallet, int* i, int* j, int* id_vpallets) //mismo de arriba pero busca en Camiones2, y entrega además el id en vextor de pallets dentro de Camion, return false si no lo encuentra
{
    for(unsigned int ii=0; ii<Camiones2.size(); ii++)
    {
        for(unsigned int jj=0; jj<Camiones2[ii].size(); jj++)
        {
            for(unsigned int kk=0; kk<Camiones2[ii][jj].carga.size(); kk++)
            {
                if(Camiones2[ii][jj].carga[kk].getid_pallet() == idPallet)
                {
                        *i = ii;
                        *j = jj;
                        *id_vpallets = kk;
                        return(true);
                }
            }
        }
    }
    
    *i = NULL;
    *j = NULL;
    *id_vpallets = NULL;
    return(false);
}                   // end searchPalletCamion

bool Edge::searchPalletCamionS(int idPallet, int* i, int* j, int* id_vpallets) //mismo de arriba pero busca en CamionesS, y entrega además el id en vextor de pallets dentro de Camion, return false si no lo encuentra
{
    for(unsigned int ii=0; ii<CamionesS.size(); ii++)
    {
        for(unsigned int jj=0; jj<CamionesS[ii].size(); jj++)
        {
            for(unsigned int kk=0; kk<CamionesS[ii][jj].carga.size(); kk++)
            {
                if(CamionesS[ii][jj].carga[kk].getid_pallet() == idPallet)
                {
                        *i = ii;
                        *j = jj;
                        *id_vpallets = kk;
                        return(true);
                }
            }
        }
    }
    
    *i = NULL;
    *j = NULL;
    *id_vpallets = NULL;
    return(false);
}                   // end searchPalletCamion

bool Edge::searchCamion(int idCamion, int* i, int* j)    //entrega posición en Camiones2, si lo encuentra
{
    if(Camiones2.size()>0)
    {
        for(unsigned int ii=0; ii<Camiones2.size(); ii++)
        {
            for(unsigned int jj=0; jj<Camiones2[ii].size(); jj++)
            {
                if(idCamion == Camiones2[ii][jj].get_idc())
                {
                    *i = ii;
                    *j = jj;
                    return(true);
                }
            }
        }
    }
  
    *i = NULL;
    *j = NULL;
    return(false);
    
}       // end search Camion

bool Edge::searchCamionS(int idCamion, int* i, int* j)    //entrega posición en CamionesS, si lo encuentra
{
    if(CamionesS.size()>0)
    {
        for(unsigned int ii=0; ii<CamionesS.size(); ii++)
        {
            for(unsigned int jj=0; jj<CamionesS[ii].size(); jj++)
            {
                if(idCamion == CamionesS[ii][jj].get_idc())
                {
                    *i = ii;
                    *j = jj;
                    return(true);
                }
            }
        }
    }
  
    *i = NULL;
    *j = NULL;
    return(false);
    
}       // end search CamionS

//busca el camión en CamionesD, Camiones2 o CamionesS, donde lo encuentre
int Edge::searchCamionTotal(int idCamion, int* i, int* j)    //entrega posición en CamionesD o Camiones 2, si lo encuentra en CamionesD, *j = NULL. Return 0 si no encuentra, 1 si encuentra en CamionesD, 2 si lo encuentra en Camiones2 y 3 si lo encuentra en CamionesS
{
    if(CamionesD.size()>0)
    {
        for(unsigned int ii=0; ii<CamionesD.size(); ii++)
        {
            if(idCamion == CamionesD[ii].get_idc())
            {
                *i = ii;
                *j = NULL;
                return(1);
            }
        }
    }       // en if si existe CamionesD
    
    if(Camiones2.size()>0)
    {
        for(unsigned int ii=0; ii<Camiones2.size(); ii++)
        {
            if(Camiones2[ii].size()>0)
            {
                for(unsigned int jj=0; jj<Camiones2[ii].size(); jj++)
                {
                    if(idCamion == Camiones2[ii][jj].get_idc())
                    {
                        *i = ii;
                        *j = jj;
                        return(2);
                    }
                }
            }
        }
    }
    
    if(CamionesS.size()>0)
    {
        for(unsigned int ii=0; ii<CamionesS.size(); ii++)
        {
            if(CamionesS[ii].size()>0)
            {
                for(unsigned int jj=0; jj<CamionesS[ii].size(); jj++)
                {
                    if(idCamion == CamionesS[ii][jj].get_idc())
                    {
                        *i = ii;
                        *j = jj;
                        return(3);
                    }
                }
            }
        }
    }
    
    
    *i = NULL;
    *j = NULL;
    return(0);
    
}       // end search Camion

// función para recorrer x2 y xR desde un arco PLanta a PLanta
void Edge::findDestinos(vector<Edge_parts>& xP, set<int>& destinos)
{
    destinos.clear();
    for(unsigned int k=0; k<xP.size(); k++)
    {
        set<int>::iterator setIter = destinos.find(xP[k].id_sucesor);
        if(setIter == destinos.end())
            destinos.insert(xP[k].id_sucesor);
        
        
        
    }    
}       // end findDestinos

// función para recorrer x2 y xR desde un arco PLanta a PLanta
void Edge::findOrigenes(vector<Camion>& cP, set<int>& origenes)
{
    origenes.clear();
    for(unsigned int k=0; k<cP.size(); k++)
    {
        set<int>::iterator setIter = origenes.find(cP[k].get_prev());
        if(setIter == origenes.end())
            origenes.insert(cP[k].get_prev());
    }
}       // end findDestinos

vector<Pallet> sortCPs(vector<Pallet> cp, bool(*fSort)(const Pallet &, const Pallet &)) {
	vector<Pallet> cp0, cp1, cpEmtre01;
	for (unsigned int i = 0; i < cp.size(); i++) {
		if (cp[i].CUAD == 0) {
			cp0.push_back(cp[i]);
		}
		else if (cp[i].CUAD == 1) {
			cp1.push_back(cp[i]);
		}
		else {
			cpEmtre01.push_back(cp[i]);
		}
	}
	sort(cp0.begin(), cp0.end(), fSort);
	sort(cp1.begin(), cp1.end(), fSort);
	sort(cpEmtre01.begin(), cpEmtre01.end(), fSort);
	cp0.insert(cp0.end(), cpEmtre01.begin(), cpEmtre01.end());
	cp0.insert(cp0.end(), cp1.begin(), cp1.end());
	return cp0;
}

void Edge::updateCarga(vector<vector<Pallet>>& CargaP, vector<Producto>& Prod, int fam)
{
    if(CargaP[fam].size() > 0)
    {
        for(unsigned int i=0; i<CargaP[fam].size(); i++)
            CargaP[fam][i].OcupPesoCuad(Prod);
		CargaP[fam] = sortCPs(CargaP[fam], &CompareOcup);
    }
    
}   // end updateCarga

void Edge::asignaPesoCajaX(vector<Edge_parts>& xP, vector<Producto>& Prod)
{
    for(unsigned int i=0; i<xP.size(); i++)
    {
        int prod_id = get_Prod_id(xP[i].atributo, Prod);
        double pc = Prod[prod_id].get_PesoCaja();
        xP[i].asignaPesoCaja(pc);
    }
}       // end AsignaPesoCajaX

void Edge::consolida_carga(vector<vector<vector<Pallet>>>& CargaP, vector<vector<Pallet>>& CargaPCons, size_t NF)
{
    CargaPCons.clear();
    CargaPCons.resize(CargaPCons.size() + NF);       // se diferencia solo por familia para asignar
    for(unsigned int k=0; k<CargaP.size(); k++)
    {
        for(unsigned int m=0; m < CargaP[k].size(); m++)
        {
            for(unsigned int l=0; l < CargaP[k][m].size(); l++)
            {
                CargaPCons[m].push_back(CargaP[k][m][l]);
            }
        }
        
    }       // end for por destino u origen, sea como esté agregado
}

void Edge::consolida_camiones(vector<vector<Camion>>& CamionesP, vector<Camion>& CamionesPCons)
{
    CamionesPCons.clear();
    if(CamionesP.size()>0)
    {
        for(unsigned int m=0; m < CamionesP.size(); m++)
            for(unsigned int l=0; l < CamionesP[m].size(); l++)
                CamionesPCons.push_back(CamionesP[m][l]);
    }
}

void Edge::desconsolida_camiones(vector<vector<Camion>>& CamionesP, vector<Camion>& CamionesPCons)
{
    CamionesP.clear();
    
    set<int> origenes;
    set<int>::iterator it;
    findOrigenes(CamionesPCons, origenes);
    
    if(origenes.size()>0)
    {
        CamionesP.resize(CamionesP.size() + origenes.size());
        int j=0;
        for (it=origenes.begin(); it!=origenes.end(); ++it)
        {
            for(unsigned int k=0; k< CamionesPCons.size(); k++)
            {
                if(CamionesPCons[k].get_prev() == *it)
                    CamionesP[j].push_back(CamionesPCons[k]);
            }
            j++;
        }
    }
}       // end función que desconsolida los caiones ya cargados

//separa CamionesPAgregado en los directos, que los pone en CamionesP de los y2, que quedan en CamionesPCons
void Edge::separa_camiones(vector<Camion>& CamionesPAgregado, vector<Camion>& CamionesP, vector<Camion>& CamionesPCons)
{
    CamionesP.clear();
    CamionesPCons.clear();
    for(unsigned int k=0; k<CamionesPAgregado.size(); k++)
    {
        if((CamionesPAgregado[k].get_prev() == -1)&&(CamionesPAgregado[k].get_nxt() == -1))
            CamionesP.push_back(CamionesPAgregado[k]);
        else
            CamionesPCons.push_back(CamionesPAgregado[k]);
    }
    CamionesPAgregado.clear();
}

//**************************** CREANDO LOS PALLETS DE CADA FAMILIA ***********************
int Edge::createPallets(int idFirstPallet, int filtro_origen, int filtro_destino, int filtro_descarga, vector<Edge_parts>& xP, vector<Edge_parts>& zP, vector<vector<Pallet>>& CargaP, vector<Producto>& Prod, vector<Familia>& FA)
{
    size_t NF = FA.size();
    CargaP.clear();
    CargaP.resize(CargaP.size() + NF);             //crea un vector de pallets por familia que son en total NF
    
    int kk = idFirstPallet;
    if(zP.size()>0)
    {
        for(unsigned int i=0; i<zP.size(); i++)
        {
            bool cond = false;
            if((zP[i].id_predec == -1)||(zP[i].id_predec == filtro_origen))
            {
                if((zP[i].id_sucesor == -1)||(zP[i].id_sucesor == filtro_destino))
                {
                    if(zP[i].descarga == filtro_descarga) //sólo se activa en viajes P-P-Szs
                        cond = true;
                }
            }
            
            if(cond)
            {
                for(unsigned int m=0; m<NF; m++)
                {
                    if(stoi(zP[i].atributo) == FA[m].get_id())
                    {
                        int TotalP = (int)zP[i].amount; // z está en pallets totales
                        
                        for(unsigned int j=0; j<TotalP; j++)
                        {
                            CargaP[m].push_back(Pallet(kk,m,zP[i].id_predec,zP[i].id_sucesor, zP[i].descarga));
                            kk++;
                        }
                    }
                }       // end for recorriendo todas las familias
                
            }   // end if cond
            
        }   // end for
        
        // escribiendo los pesos de caja en los xP
        asignaPesoCajaX(xP,Prod);
        
    }// END IF
    return(kk);
}                   // end createPallets()

//******************* ALGORITMOS QUE CARGAN PRODUCTOS A PALLETS ***************************
//*************************************************************++************************

void Edge::ProdPallet(double Amount, int i, string type, vector<Pallet>& CargaP, Producto Prod, vector<Edge_parts>& Sobrante, int frescura, bool super, int id_pred, int id_suc, int id_ep, int node_id, int des)
{
   bool cuadratura = false;
   if((type == "qx")||(type == "qx2")||(type == "qxr")||(type == "qxs"))
        cuadratura = true;
    double CapacKilosPallets = Factor*CapKilosTrucks();
    double CapacPeso = (double)CapacKilosPallets/(double)Prod.get_PesoCaja();
    double Capac_pallet = min(Prod.get_CajasPallet(),CapacPeso);
    
    double Remanente;
    if(cajas_enteras)  {           //****ESTE REMANENTE TIENE QUE SER ENTERO EN ESTA OPCIÓN
        Remanente = floor((1-CargaP[i].ocupacion)*Capac_pallet);
    }
    else {
        Remanente = (1-CargaP[i].ocupacion)*Capac_pallet;
    }
    
    double Delta = (double)Amount/(double)Remanente;
    
    if(Delta > 1) 
    {
        CargaP[i].BC.push_back(Bunch_cajas(Prod.getid(),Remanente, frescura, super, cuadratura,des));
        double DeltaPorCargar = Amount - Remanente;
        
        if((i+1) < CargaP.size())
            ProdPallet(DeltaPorCargar, i + 1, type, CargaP, Prod, Sobrante, frescura, super, id_pred, id_suc, id_ep, node_id, des);
        else
        {
            double cpp = Prod.get_CajasPallet();
            Sobrante.push_back(Edge_parts(type,Prod.get_code(),DeltaPorCargar,frescura,super,Prod.get_PesoCaja(),-1,id_pred,id_suc,node_id,des,cpp));
            Sobrante.back().id_ep = id_ep;

        }
    }
    else
       CargaP[i].BC.push_back(Bunch_cajas(Prod.getid(),Amount, frescura, super, cuadratura, des));
    
}       // end ProdPallet

vector<Edge_parts> sortXs(vector<Edge_parts> x) {
	vector<Edge_parts> xBajo;
	vector<Edge_parts> xSobre;

	for (auto e = x.begin(); e != x.end(); e++) {
		if ((*e).pesoCaja > UmbralPesoCaja) {
			xSobre.push_back(*e);
		} else {
			xBajo.push_back(*e);
		}
	}
	sort(xSobre.begin(), xSobre.end());
	sort(xBajo.begin(), xBajo.end());
	xSobre.insert(xSobre.end(), xBajo.begin(), xBajo.end());
	return xSobre;
}

vector<Edge_parts> sortXsCajaPallets(vector<Edge_parts> x, bool ascendente) {
	map<int, vector<Edge_parts>> mXs;
	vector<Edge_parts> vXs;
	vector<int> vKeys;

	for (auto e = x.begin(); e != x.end(); e++) {
		if (mXs.find((*e).CajasPorPallet) != mXs.end()) {
			mXs[(*e).CajasPorPallet].push_back(*e);
		}
		else {
			vector<Edge_parts> xs;
			xs.push_back(*e);
			mXs.insert({ (*e).CajasPorPallet, xs });
		}
	}

	map<int, vector<Edge_parts>>::iterator it = mXs.begin();
	for (pair < int, vector<Edge_parts >> element : mXs) {
		vKeys.push_back(element.first);
	}
	if (ascendente) {
		sort(vKeys.begin(), vKeys.end());
	}
	else {
		reverse(vKeys.begin(), vKeys.end());
	}
	for (auto e = vKeys.begin(); e != vKeys.end(); e++) {
		sortXs(mXs[(*e)]);
		vXs.insert(vXs.end(), mXs[(*e)].begin(), mXs[(*e)].end());
	}
	return vXs;
}


vector<Edge_parts> sortXsCUAD(vector<Edge_parts> x) {
	vector<Edge_parts> xConCUAD;
	vector<Edge_parts> xSinCUAD;

	for (auto e = x.begin(); e != x.end(); e++) {
		if ((*e).type[0] == 'q') {
			xConCUAD.push_back(*e);
		}
		else {
			xSinCUAD.push_back(*e);
		}
	}
	vector<Edge_parts> xSinCUADSort = sortXsCajaPallets(xSinCUAD, Ascendente);// sortXs(xSinCUAD);
	vector<Edge_parts> xConCUADSort = sortXsCajaPallets(xConCUAD, Ascendente);// sortXs(xConCUAD);
	xSinCUADSort.insert(xSinCUADSort.end(), xConCUADSort.begin(), xConCUADSort.end());
	return xSinCUADSort;
}

void Edge::CargaAmountEP(Edge_parts xPc, vector<Producto>& Prod, vector<vector<Pallet>>& CargaP, vector<Edge_parts>& Sobrante)
{
    int idP = get_Prod_id(xPc.atributo,Prod);
    double Amount = xPc.amount;
    int Fam = Prod[idP].get_familia();
    
    updateCarga(CargaP,Prod,Fam);
    if(CargaP[Fam].size()>0)
        ProdPallet(Amount,0,xPc.type, CargaP[Fam], Prod[idP], Sobrante, xPc.frescura, xPc.SuperMercado, xPc.id_predec, xPc.id_sucesor, xPc.id_ep, xPc.node_id, xPc.descarga);
    
}       // end fn auxiliar para cargar

//cuenta las cajas de Sobrante
int NcajasSobrante(vector<Edge_parts>& Sobrante)
{
    int cajas = 0;
    if(Sobrante.size()>0)
    {
        for(unsigned int k=0; k<Sobrante.size();k++)
            cajas += Sobrante[k].amount;
    }
        return cajas;
}

// carga los pallets a los productos
int Edge::cargaPalletsProductos(int idFirstPallet, vector<Producto>& Prod, vector<Edge_parts>& xP, vector<Edge_parts>& zP, vector<vector<Pallet>>& CargaP, int filtro_origen, int filtro_destino, int filtro_descarga, vector<Edge_parts>& Sobrante, vector<Familia>& FA)
{
    
    Sobrante.clear();
    size_t NF = FA.size();
    vector<vector<Pallet>> CargaPAux;
    vector<Edge_parts> SobranteAux;
    
    int lastPallet = createPallets(idFirstPallet, filtro_origen, filtro_destino, filtro_descarga, xP, zP, CargaPAux, Prod, FA);

    
    if(xP.size()>0)
    {
		xP = sortXsCUAD(xP);// sort(xP.begin(), xP.end());           // ordeno x de mayor a menor
        for(unsigned int i=0; i<xP.size(); i++)
        {
            bool cond = false;
            if((xP[i].id_predec == -1)||(xP[i].id_predec == filtro_origen))
            {
                if((xP[i].id_sucesor == -1)||(xP[i].id_sucesor == filtro_destino))
                {
                    if(xP[i].descarga == filtro_descarga)
                        cond = true;
                }
            }

            
            if(cond)
                CargaAmountEP(xP[i],Prod,CargaPAux,SobranteAux);
        }       // end for i
        
        if((NcajasSobrante(SobranteAux) == 0)||(xP.size()<PGRASP))
        {
            CargaP = CargaPAux;
            Sobrante = SobranteAux;
        }
        else    //implemento GRASP a ver si subo el sobrante a los pallets
        {
            srand((unsigned)time(NULL));
            
            vector<vector<Pallet>> CargaPMejor = CargaPAux;
            vector<Edge_parts> SobranteMejor = SobranteAux;
            
            //número de iteraciones del GRASP
            for(unsigned int g=0; g<ITER_GRASP; g++)
            {
                lastPallet = createPallets(idFirstPallet, filtro_origen, filtro_destino, filtro_descarga, xP, zP, CargaPAux, Prod, FA);
                bool NotAssigned = true;
                set<int> counter;       //mantiene los xP que voy prendiendo
                set<int>::iterator it;
                while(NotAssigned)
                {
                    vector<int> GRASP_Candidates;
                    for(unsigned int i=0; i<xP.size(); i++)
                    {
                        bool cond = false;
                        bool cond2 = counter.find(i) != counter.end();
                        
                        if((xP[i].id_predec == -1)||(xP[i].id_predec == filtro_origen))
                        {
                            if((xP[i].id_sucesor == -1)||(xP[i].id_sucesor == filtro_destino))
                            {
                                if(xP[i].descarga == filtro_descarga)
                                    cond = true;
                            }
                        }

                        if((cond)&&(!cond2))
                            GRASP_Candidates.push_back(i);
                        if(GRASP_Candidates.size() == PGRASP)
                            break;
                    }   // end for
                    size_t NN = GRASP_Candidates.size();
                    if(NN == 0) {
                        NotAssigned = false;
                    }
                    else {
                        int Rnumber = rand() % NN;  // si NN = 3, genera RV 0,1,2
                        CargaAmountEP(xP[GRASP_Candidates[Rnumber]],Prod,CargaPAux,SobranteAux);
                        counter.insert(GRASP_Candidates[Rnumber]);
                    }
                }       // end while not assigned
                
                if(NcajasSobrante(SobranteAux) < NcajasSobrante(SobranteMejor))
                {
                    SobranteMejor = SobranteAux;
                    CargaPMejor = CargaPAux;
                }
                
                SobranteAux.clear();
                CargaPAux.clear();
                
            }       // end for iterando con el GRASP
            
            CargaP = CargaPMejor;
            Sobrante = SobranteMejor;
            
        }       // end else GRASP
        
    }   // end xP > 0

    // acá se actualiza la ocupación, peso y datos de cuadratura de los pallets
    
    for(unsigned int k=0; k<NF; k++)
    {
        for(unsigned int i=0; i<CargaP[k].size(); i++)
            CargaP[k][i].OcupPesoCuad(Prod);
    }

    return(lastPallet);
}       // end carga Pallets a productos


//**************************** CREANDO LOS CAMIONES **************************************

int Edge::createCamiones(int idFirstCamion, int filtro_origen, int filtro_destino, vector<Edge_parts>& yP, vector<Camion>& CamionesP, vector<Node>& Nodes, size_t NF)
{
    int kk = idFirstCamion;
    
    if(yP.size()>0)
    {
        for(unsigned int i=0; i<yP.size(); i++)
        {
            string Clasif;
            if((Nodes[unode].get_type() == 1) && (Nodes[dnode].get_type() == 1) && (yP[i].type == "y"))
               Clasif = "interplanta";
            else
               Clasif = "primario";
               
            bool cond = false;
            
    //        cout << "cond: " << yP[i].id_predec << ":" << yP[i].id_sucesor << endl;
            
            if((yP[i].id_predec == -1)||(yP[i].id_predec == filtro_origen))
                if((yP[i].id_sucesor == -1)||(yP[i].id_sucesor == filtro_destino))
                    cond = true;
            
            if(cond)
            {
                    int TotalC = (int)yP[i].amount;
  
                    for(unsigned int j=0; j<TotalC; j++)
                    {
                        CamionesP.push_back(Camion(kk, yP[i].atributo, edge_id, yP[i].id_predec, yP[i].id_sucesor, Clasif, NF, transportes[yP[i].atributo].volumen, transportes[yP[i].atributo].peso, transportes[yP[i].atributo].minP, transportes[yP[i].atributo].maxP));
                        kk++;
                    }
            }
           
        }   // end for
    }// END IF
    
    return(kk);
}                   // end createCamiones()

//*************************** ALGORITMO QUE CARGA PALLETS A CAMIONES *****************************
//************************************************************************************************
void Edge::LoadCamiones(vector<Camion>& CamionesP, int k, vector<Pallet>& P1, int p1_id, vector<Pallet>& P2, int p2_id, vector<Producto>& Prod, vector<trioPallet>& sobranteP)
{
    double PesoExtra = P1[p1_id].peso + P2[p2_id].peso;
    bool RestrP = CamionesP[k].sobreCargaP(2);
    bool RestrK = CamionesP[k].sobreCargaK(PesoExtra, Prod);
    bool condf = CamionesP[k].Fresco1capa;
        
    if((!RestrP)&&(!RestrK)&&(!condf))
    {
        CamionesP[k].carga.push_back(P1[p1_id]);
        CamionesP[k].carga.back().set_camion(CamionesP[k].get_idc());
        CamionesP[k].PalletsPorFamilia[P1[p1_id].familia]++;
        P1[p1_id].set_camion(k);
        CamionesP[k].carga.push_back(P2[p2_id]);
        CamionesP[k].carga.back().set_camion(CamionesP[k].get_idc());
        CamionesP[k].PalletsPorFamilia[P2[p2_id].familia]++;
        P2[p2_id].set_camion(k);
        CamionesP[k].computePesoOcupacion();
        
    }       // end if
    else
    {
        if((k+1) < CamionesP.size())
            LoadCamiones(CamionesP, k+1, P1, p1_id, P2,p2_id, Prod, sobranteP);
        else
        {
            sobranteP.push_back(trioPallet(P1[p1_id].familia, p1_id, P1[p1_id].peso));
            sobranteP.push_back(trioPallet(P2[p2_id].familia, p2_id, P2[p2_id].peso));
        }
    }       // end else
}       // end LoadCamiones

void Edge::LoadCamionesSingle(vector<Camion>& CamionesP, int k, vector<Pallet>& P1, int p1_id, vector<Producto>& Prod, vector<trioPallet>& sobranteP, vector<Familia>& FA)
{
    double PesoExtra = P1[p1_id].peso;
    bool RestrP = CamionesP[k].sobreCargaP(1);
    bool RestrK = CamionesP[k].sobreCargaK(PesoExtra, Prod);
    
    bool cong = FA[P1[p1_id].familia].congelado;
//    bool cong = (P1[p1_id].getf() == 2) || (P1[p1_id].getf() == 3) || (P1[p1_id].getf() == 4);
    
    bool condf = (CamionesP[k].Fresco1capa)&&(cong);
    
    if((!RestrP)&&(!RestrK)&&(!condf))
    {
        CamionesP[k].carga.push_back(P1[p1_id]);
        CamionesP[k].carga.back().set_camion(CamionesP[k].get_idc());
        CamionesP[k].PalletsPorFamilia[P1[p1_id].familia]++;
        P1[p1_id].set_camion(k);
        CamionesP[k].computePesoOcupacion();
        
    }       // end if
    else
    {
        if((k+1) < CamionesP.size())
            LoadCamionesSingle(CamionesP, k+1, P1, p1_id, Prod, sobranteP,FA);
        else
            sobranteP.push_back(trioPallet(P1[p1_id].familia, p1_id, P1[p1_id].peso));
    }       // end else
}       // end LoadCamiones

//función para ordenar vector de Camiones según fammilia que corresponde
void OrdenaCamionesFamilia(int familia, vector<Camion>& CamionesP)
{
    for(unsigned int pp=0; pp<CamionesP.size(); pp++)
        CamionesP[pp].computePesoOcupacion();
    
    if(NoFamilia)
        sort(CamionesP.begin(),CamionesP.end(),&CompareCamion);
    else
    {
        if(familia == 0)
            sort(CamionesP.begin(),CamionesP.end(),&CompareCamionF0);
        else if(familia == 1)
            sort(CamionesP.begin(),CamionesP.end(),&CompareCamionF1);
        else if(familia == 2)
            sort(CamionesP.begin(),CamionesP.end(),&CompareCamionF2);
        else if(familia == 3)
            sort(CamionesP.begin(),CamionesP.end(),&CompareCamionF3);
        else if(familia == 4)
            sort(CamionesP.begin(),CamionesP.end(),&CompareCamionF4);
        else if(familia == 5)
            sort(CamionesP.begin(),CamionesP.end(),&CompareCamionF5);
        else if(familia == 6)
            sort(CamionesP.begin(),CamionesP.end(),&CompareCamionF6);
        else if(familia == 7)
            sort(CamionesP.begin(),CamionesP.end(),&CompareCamionF7);
        else if(familia == 8)
            sort(CamionesP.begin(),CamionesP.end(),&CompareCamionF8);
        else if(familia == 9)
            sort(CamionesP.begin(),CamionesP.end(),&CompareCamionF9);
        else
            sort(CamionesP.begin(),CamionesP.end(),&CompareCamion);
    }
        
}

// fn que carga vector de productos congelados en vector de camiones, entra com input el hecho de traer o no fresco
void Edge::Carga_CongeladoV0(vector<Camion>& CamionesP, vector<Pallet>& CargaCongelados, bool HayFresco, size_t SizeCarga2, vector<trioPallet>& sobranteC, vector<Producto>& Prod)
{
    // +++++++++++++++++++++ cargando congelados ++++++++++++++++++++++++++++
    if(CamionesP.size() > 0)
    {
        if(NoFamilia)
            OrdenaCamionesFamilia(0,CamionesP);
        for(unsigned int k=0; k<SizeCarga2; k++)
        {
            if(!NoFamilia)
            {
                int fam = CargaCongelados[2*k].familia;
                OrdenaCamionesFamilia(fam,CamionesP);
            }
            LoadCamiones(CamionesP,0,CargaCongelados, 2*k, CargaCongelados, 2*k+1, Prod, sobranteC);
        }
        // AJUSTA CONGELADOS EN CASO QUE HAYA FRESCO QUE CARGAR
        if(HayFresco)
        {
            int nit = 0;
            bool cond = AjustaCamiones(CamionesP, Prod);
            
            while((!cond) && (nit < ITERAJUSTECong))
            {
                nit++;
                cond = AjustaCamiones(CamionesP, Prod);

            }
        }
    }
    else
    {
        for(unsigned int m=0; m<CargaCongelados.size(); m++)
            sobranteC.push_back(trioPallet(CargaCongelados[m].familia, m, CargaCongelados[m].peso));
    }
}       // end fn Carga_CongeladoV0

// fn que carga vector de productos congelados en vector de camiones, entra com input el hecho de traer o no fresco
void Edge::Carga_CongeladoV1(vector<Camion>& CamionesP, vector<Pallet>& CargaCongelados, bool HayFresco, size_t SizeCarga2, vector<trioPallet>& sobranteC, vector<Producto>& Prod)
{
    // +++++++++++++++++++++ cargando congelados ++++++++++++++++++++++++++++
    if(CamionesP.size() > 0)    //acá cargo para tratar de cumplir las condiciones de mínimo y máximo para congelados. Se corre ajuste de congelados, sólo en caso de tener productos frescos en este mismo arco
    {
        for(unsigned int pp=0; pp<CamionesP.size(); pp++)
            CamionesP[pp].computePesoOcupacion();
        sort(CamionesP.begin(),CamionesP.end(),&CompareCamion);
              
        int i=0;
        int MaxCarga = CamionesP[i].maxPalletsC;
        
        for(unsigned int k=0; k<SizeCarga2; k++)
        {
            LoadCamiones(CamionesP,i,CargaCongelados, 2*k, CargaCongelados, 2*k+1, Prod, sobranteC);
            for(unsigned int pp=0; pp<CamionesP.size(); pp++)
                CamionesP[pp].computePesoOcupacion();
            if(CamionesP[i].carga.size() == MaxCarga)
            {
                i++;
                if(i<CamionesP.size())
                {
                    if(CamionesP[i].carga.size() >= CamionesP[i].maxPalletsC)
                        MaxCarga = CamionesP[i].capacP;
                    else
                        MaxCarga = CamionesP[i].maxPalletsC;
                }
            }   // end if
            if(i==CamionesP.size())
            {
                i = 0;
                if(CamionesP[i].carga.size() >= CamionesP[i].maxPalletsC)
                    MaxCarga = CamionesP[i].capacP;
                else
                    MaxCarga = CamionesP[i].maxPalletsC;
            }   // end if
        }       // end for k para carga de pallets

        // AJUSTA CONGELADOS EN CASO QUE HAYA FRESCO QUE CARGAR
        if(HayFresco)
        {
            int nit = 0;
            bool cond = AjustaCamiones(CamionesP, Prod);
            
            while((!cond) && (nit < ITERAJUSTECong))
            {
                nit++;
                cond = AjustaCamiones(CamionesP, Prod);
            }
        }
    }       //  fin caso donde cargo primer arco PP o PS, acá debo imponer condición mínimo y máximo para congelados
    else
    {
        for(unsigned int m=0; m<CargaCongelados.size(); m++)
            sobranteC.push_back(trioPallet(CargaCongelados[m].familia, m, CargaCongelados[m].peso));
    }
}       // end fn Carga_CongeladoV1

// fn que carga vector de productos congelados en vector de camiones, entra com input el hecho de traer o no fresco
void Edge::Carga_CongeladoV2(vector<Camion>& CamionesP, vector<Pallet>& CargaCongelados, bool HayFresco, size_t SizeCarga2, vector<trioPallet>& sobranteC, vector<Producto>& Prod)
{
    // +++++++++++++++++++++ cargando congelados ++++++++++++++++++++++++++++
    if(CamionesP.size() > 0)    //acá cargo para tratar de cumplir las condiciones de mínimo y máximo para congelados. Se corre ajuste de congelados, sólo en caso de tener productos frescos en este mismo arco
    {

        if(NoFamilia)
            OrdenaCamionesFamilia(0,CamionesP);
        for(unsigned int k=0; k<SizeCarga2; k++)
        {
            if(!NoFamilia)
            {
                int fam = CargaCongelados[2*k].familia;
                OrdenaCamionesFamilia(fam,CamionesP);
            }
            LoadCamiones(CamionesP,0,CargaCongelados, 2*k, CargaCongelados, 2*k+1, Prod, sobranteC);
        }
        
        // AJUSTA CONGELADOS EN CASO QUE HAYA FRESCO QUE CARGAR
        if(HayFresco)
        {
            int nit = 0;
            bool cond = AjustaCamiones2(CamionesP, Prod);
            while((!cond) && (nit < ITERAJUSTECong))
            {
                nit++;
                cond = AjustaCamiones2(CamionesP, Prod);
            }
        }
    }       //  fin caso donde cargo primer arco PP o PS, acá debo imponer condición mínimo y máximo para congelados
    else
    {
        for(unsigned int m=0; m<CargaCongelados.size(); m++)
            sobranteC.push_back(trioPallet(CargaCongelados[m].familia, m, CargaCongelados[m].peso));
    }
}       // end fn Carga_CongeladoV2

// esta fn recibe como inputvector CargaP que hay que subir a los camiones, que se crean en el arco además de los que podrian venir con algún espacio desde otro arco CamionesPrevios
int Edge::LoadCamionesEdge(int idFirstCamion, vector<Camion>& CamionesPrevios, vector<Camion>& CamionesP, vector<vector<Pallet>>& CargaP1, vector<vector<Pallet>>& CargaP2, vector<Edge_parts>& yP, vector<Producto>& Prod, vector<Node>& Nodes, int  filtro_origen, int filtro_destino, vector<Pallet>& sobranteF, bool consolidacion,  vector<Familia>& FA)
{
    // Primero se generan los vectores por separado de carga congelados y carga no congelados
    // Partimos generando vector de congelados
    vector<Pallet> CargaCongelados;
    CargaCongelados.clear();
    
    if(CargaP1.size() > 0)
    {
        for(unsigned int i=0; i<FA.size(); i++)
        {
           if((FA[i].congelado)&&(CargaP1[i].size()>0))
           {
               for(unsigned int j=0; j<CargaP1[i].size(); j++)
               {

                  if((CargaP1[i][j].get_next_edge() == -1)||(CargaP1[i][j].get_next_edge() == filtro_destino)||(consolidacion))
                 {
                     if((CargaP1[i][j].get_prev_edge() == -1)||(CargaP1[i][j].get_prev_edge() == filtro_origen)||(consolidacion))
                                  CargaCongelados.push_back(CargaP1[i][j]);
                 }
               }
           }    // end if
            
        }   // end for recorriendo familias
    }       // end if CargaP1.size() > 0
        
    if(CargaP2.size() > 0)
    {
        for(unsigned int i=0; i<FA.size(); i++)
        {
           if((FA[i].congelado)&&(CargaP2[i].size()>0))
           {
               for(unsigned int j=0; j<CargaP2[i].size(); j++)
               {
                   if((CargaP2[i][j].get_next_edge() == -1)||(CargaP2[i][j].get_next_edge() == filtro_destino)||(consolidacion))
                   {
                       if((CargaP2[i][j].get_prev_edge() == -1)||(CargaP2[i][j].get_prev_edge() == filtro_origen)||(consolidacion))
                                      CargaCongelados.push_back(CargaP2[i][j]);
                   }
                   
               }
           }        // end if
        }           // end for
        
    }       // end if
    
    // ahora se genera vector de productos no congelados
    vector<Pallet> CargaNoCongelados;
    CargaNoCongelados.clear();
    
    if(CargaP1.size() > 0)
    {
        for(unsigned int i=0; i<FA.size(); i++)
        {
           if((!FA[i].congelado)&&(CargaP1[i].size()>0))
           {
               for(unsigned int j=0; j<CargaP1[i].size(); j++)
               {
                  if((CargaP1[i][j].get_next_edge() == -1)||(CargaP1[i][j].get_next_edge() == filtro_destino)||(consolidacion))
                  {
                     if((CargaP1[i][j].get_prev_edge() == -1)||(CargaP1[i][j].get_prev_edge() == filtro_origen)||(consolidacion))
                                    CargaNoCongelados.push_back(CargaP1[i][j]);
                  }
               }
           }      // end if
        }         // end for
    }             // end if
  
    if(CargaP2.size() > 0)
    {
        for(unsigned int i=0; i<FA.size(); i++)
        {
           if((!FA[i].congelado)&&(CargaP2[i].size()>0))
           {
               for(unsigned int j=0; j<CargaP2[i].size(); j++)
               {
                   if((CargaP2[i][j].get_next_edge() == -1)||(CargaP2[i][j].get_next_edge() == filtro_destino)||(consolidacion))
                   {
                       if((CargaP2[i][j].get_prev_edge() == -1)||(CargaP2[i][j].get_prev_edge() == filtro_origen)||(consolidacion))
                                       CargaNoCongelados.push_back(CargaP2[i][j]);
                   }
               }
               
           }      // end if
        }         // end for
    }             // end if
  
    //++++++++++++++ AHORA SE CARGAN LOS CONGELADOS PRIMERO DE A PARES Y RESPETANDO CONDICION SI VECTOR DE NO CONGELADOS TIENE TAMAÑO > 0+++++++++++++++++++++++++++
    bool HayFresco = true;
    if(CargaNoCongelados.size()==0)
        HayFresco = false;
    //ordena los pallets con criterio de familia primero y peso después (mayor peso primero)
    CargaCongelados = sortCPs(CargaCongelados, &CompareFam);
    
    // creo los camiones requeridos para ese arco, si no hay
    int idLastCam = createCamiones(idFirstCamion,filtro_origen,filtro_destino,yP,CamionesP,Nodes,FA.size());
    
    vector<trioPallet> sobrante;
    sobrante.clear();
    vector<trioPallet> sobrante2;
    sobrante2.clear();
    vector<trioPallet> sobrante3;
    sobrante3.clear();
    vector<trioPallet> sobrante4;
    sobrante4.clear();
    vector<trioPallet> sobrante5;
    sobrante5.clear();
   
    // SE CARGA DE A PARES DE PALLETS CONGELADOS, PRIMERO TODAS LAS FAMILIAS CONGELADOS, DE AHÍ SE REORDENA LA LISTA DE CAMIONES, Y SE SIGUE CARGANDO EL RESTO DE LAS FAMILIAS DE A UN PALLET A LA VEZ
    
    int resto = CargaCongelados.size() % 2;
    if(resto != 0)      // número impar de pallets congelado
        CargaCongelados.push_back(Pallet(-100, 4, -1, -1, -1));   //pallet congelado dummy (de familia 5 con id 4)
        
      int SizeCarga2 = floor(0.5*CargaCongelados.size());
    
    if(COND_CONGELADO == 0)
        Carga_CongeladoV0(CamionesP, CargaCongelados, HayFresco, SizeCarga2, sobrante, Prod);
    else if(COND_CONGELADO == 1)
        Carga_CongeladoV1(CamionesP, CargaCongelados, HayFresco, SizeCarga2, sobrante, Prod);
    else if(COND_CONGELADO == 2)
        Carga_CongeladoV2(CamionesP, CargaCongelados, HayFresco, SizeCarga2, sobrante, Prod);
   
    if(!HayFresco)
    {
        if((CamionesPrevios.size() > 0) && (sobrante.size() > 1))
        {
           sort(sobrante.begin(),sobrante.end(),&CompareFamTrio);
           int SizeCarga22 = floor(0.5*sobrante.size());
           
            if(NoFamilia)
                OrdenaCamionesFamilia(0,CamionesPrevios);
           for(unsigned int k=0; k<SizeCarga22; k++)
            {
                if(!NoFamilia)
                {
                    int fam = sobrante[2*k].fam;
                    OrdenaCamionesFamilia(fam,CamionesPrevios);
                }
                LoadCamiones(CamionesPrevios,0,CargaCongelados, sobrante[2*k].idPallet, CargaCongelados, sobrante[2*k+1].idPallet, Prod, sobrante2);
            }
        }
        
        else if((CamionesPrevios.size() > 0) && (sobrante.size() == 1))
        {
           int fam = sobrante[0].fam;
           OrdenaCamionesFamilia(fam,CamionesPrevios);
            
            LoadCamionesSingle(CamionesPrevios, 0, CargaCongelados,sobrante[0].idPallet,Prod,sobrante3, FA);
        }
        else if((CamionesPrevios.size() == 0) && (sobrante.size() > 0))
            sobrante2 = sobrante;
    }       // termino de chequear si no hay fresco
    else
        sobrante2 = sobrante;
    
    //++++++++++ acá se arregla el posible caso donde quede algún camión cargado con menos que el mínimo de congelados, el modelo en este caso debería tener holgura para cargar fresco, así que se rellena con pallets dummies
    for(unsigned int k=0; k<CamionesP.size(); k++)
    {
        if((CamionesP[k].nPallets() < CamionesP[k].minPalletsC) && (CamionesP[k].nPallets() > 0))
        {
            size_t palletsParaAgregar = CamionesP[k].minPalletsC - CamionesP[k].nPallets();
            for(unsigned int m=0; m<palletsParaAgregar; m++)
                CamionesP[k].carga.push_back(Pallet(-100, 4, -1, -1, -1));
        }
    }       // end for
    
    //++++++++++ todos los pallets de productos NO congelados se cargan a continuación de a uno a la vez +++++++++
     
    //ordena los pallets conc criterio de peso (mayor peso primero)
  //  CargaNoCongelados = sortCPs(CargaNoCongelados, &ComparePeso);
    sort(CargaNoCongelados.begin(),CargaNoCongelados.end(),&CompareFam);
    
    //cargando los productos no congelados
    if(CamionesP.size()>0)
    {
        if(NoFamilia)
            OrdenaCamionesFamilia(0,CamionesP);
        for(unsigned int j=0; j<CargaNoCongelados.size(); j++)
        {
            if(!NoFamilia)
            {
                int fam = CargaNoCongelados[j].familia;
                OrdenaCamionesFamilia(fam,CamionesP);
            }
            LoadCamionesSingle(CamionesP, 0, CargaNoCongelados,j,Prod,sobrante4, FA);
        }
    }
    else
    {
        for(unsigned int m=0; m<CargaNoCongelados.size(); m++)
            sobrante4.push_back(trioPallet(CargaNoCongelados[m].familia, m, CargaNoCongelados[m].peso));
    }
    
    // cargo en la segunda flota lo que queda remanente
    if((CamionesPrevios.size() > 0) && (sobrante4.size() > 0))
    {
        sort(sobrante4.begin(),sobrante4.end(),&CompareFamTrio);
        for(unsigned int k=0; k<sobrante4.size(); k++)
        {
            int fam = sobrante4[k].fam;
            OrdenaCamionesFamilia(fam,CamionesPrevios);
            LoadCamionesSingle(CamionesPrevios,0,CargaNoCongelados, sobrante4[k].idPallet, Prod, sobrante5, FA);
        }
    }
    else if((CamionesPrevios.size() == 0) && (sobrante4.size() > 0))
        sobrante5 = sobrante4;
    
    // pallets que no se pudieron cargar
    sobranteF.clear();
    for(unsigned int k=0; k<sobrante2.size(); k++)
        sobranteF.push_back(CargaCongelados[sobrante2[k].idPallet]);
    for(unsigned int k=0; k<sobrante3.size(); k++)
        sobranteF.push_back(CargaCongelados[sobrante3[k].idPallet]);
    for(unsigned int k=0; k<sobrante5.size(); k++)
        sobranteF.push_back(CargaNoCongelados[sobrante5[k].idPallet]);
    
    return(idLastCam);
    
}       // end LoadCamionesEdge

bool Edge::CheckPalletCargado(int search_idP)
{
    if(CamionesD.size() > 0)
    {
        for(unsigned int j=0; j<CamionesD.size(); j++)
        {
            if(CamionesD[j].carga.size()>0)
            {
                for(unsigned l=0; l<CamionesD[j].carga.size(); l++)
                    if(CamionesD[j].carga[l].getid_pallet() == search_idP)
                        return(true);
            }
                  
        }
    }       //check camiones D
    
    if(Camiones2.size() > 0)
    {
        for(unsigned int j=0; j<Camiones2.size(); j++)
        {
            if(Camiones2[j].size() > 0)
            {
                for(unsigned k=0; k<Camiones2[j].size(); k++)
                {
                    if(Camiones2[j][k].carga.size()>0)
                    {
                        for(unsigned l=0; l<Camiones2[j][k].carga.size(); l++)
                            if(Camiones2[j][k].carga[l].getid_pallet() == search_idP)
                                return(true);
                    }
                       
                }
            }
               
        }
    }       //check camiones 2
    
    if(CamionesS.size() > 0)
    {
        for(unsigned int j=0; j<CamionesS.size(); j++)
        {
            if(CamionesS[j].size() > 0)
            {
                for(unsigned k=0; k<CamionesS[j].size(); k++)
                {
                    if(CamionesS[j][k].carga.size()>0)
                    {
                        for(unsigned l=0; l<CamionesS[j][k].carga.size(); l++)
                            if(CamionesS[j][k].carga[l].getid_pallet() == search_idP)
                                return(true);
                    }
                       
                }
            }
               
        }
    }       //check camiones S
    
    return(false);
}       // end CheckPalletCarga

// imprime detalle de pallets no cargados en este arco
string Edge::PalletsNoCargados(vector<Node>& Nodes, vector<Producto>& Prod)
{
    stringstream s;
    s << "E:" << edge_id << "  NO: " << Nodes[unode].get_cod() << " ND: " << Nodes[dnode].get_cod() << endl;
    s << "  PALLETS NO CARGADOS" << endl;
    
    bool cond;
    bool condf = false;
    
    if(CargaD.size()>0)
    {
        s << "Pallets en CargaD" << endl;
        for(unsigned int k=0; k<CargaD.size(); k++)
        {
            if(CargaD[k].size()>0)
            {
                  for(unsigned int l=0; l<CargaD[k].size(); l++)
                  {
                      cond = CheckPalletCargado(CargaD[k][l].getid_pallet());
                      if(!cond)
                      {
                          s << CargaD[k][l].Print_pallet(Prod);
                          condf = true;
                      }
                  }
            }
              
        }
    }           // end if CargaD
    
    if(Carga2.size()>0)
    {
        s << "Pallets en Carga2" << endl;
        for(unsigned int k=0; k<Carga2.size(); k++)
        {
            if(Carga2[k].size()>0)
            {
                for(unsigned int l=0; l<Carga2[k].size(); l++)
                {
                    if(Carga2[k][l].size()>0)
                    {
                        for(unsigned int m=0; m<Carga2[k][l].size(); m++)
                        {
                            cond = CheckPalletCargado(Carga2[k][l][m].getid_pallet());
                            if(!cond)
                            {
                                s << Carga2[k][l][m].Print_pallet(Prod);
                                condf = true;
                            }
                        }
                    }
                }
                  
            }
            
        }
    }           // end if Carga2
    
    if(CargaR.size()>0)
    {
        s << "Pallets en CargaR" << endl;
        for(unsigned int k=0; k<CargaR.size(); k++)
        {
            if(CargaR[k].size()>0)
            {
                for(unsigned int l=0; l<CargaR[k].size(); l++)
                {
                    if(CargaR[k][l].size()>0)
                    {
                        for(unsigned int m=0; m<CargaR[k][l].size(); m++)
                        {
                            cond = CheckPalletCargado(CargaR[k][l][m].getid_pallet());
                            if(!cond)
                            {
                                s << CargaR[k][l][m].Print_pallet(Prod);
                                condf = true;
                            }
                        }
                    }
                }
                  
            }
            
        }
    }           // end if CargaR
    
    if(CargaS.size()>0)
    {
        s << "Pallets en CargaS" << endl;
        for(unsigned int k=0; k<CargaS.size(); k++)
        {
            if(CargaS[k].size()>0)
            {
                for(unsigned int l=0; l<CargaS[k].size(); l++)
                {
                    if(CargaS[k][l].size()>0)
                    {
                        for(unsigned int m=0; m<CargaS[k][l].size(); m++)
                        {
                            cond = CheckPalletCargado(CargaS[k][l][m].getid_pallet());
                            if(!cond)
                            {
                                s << CargaS[k][l][m].Print_pallet(Prod);
                                condf = true;
                            }
                        }
                    }
                }
                  
            }
            
        }
    }           // end if CargaS
    
    if(condf)
        return s.str();
    else
        return ("CARGA COMPLETA");
}       // end fn que imprime pallets no cargados

//cuenta número de pallets de un cierto producto id_producto llegando a nodo fin de este edge
double Edge::Pallets_producto(string id_producto, vector<Node>& Nodes, vector<Producto>& Prod)
{
    double sum = 0;
    //primero chequeo camiones directos
    if(CamionesD.size()>0)
    {
        for(unsigned int j=0; j<CamionesD.size(); j++)
        {
            if(CamionesD[j].carga.size()>0)
            {
                for(unsigned int k=0; k<CamionesD[j].carga.size(); k++)
                {
                    if(CamionesD[j].carga[k].BC.size() > 0)
                    {
                        for(unsigned int l=0; l<CamionesD[j].carga[k].BC.size(); l++)
                        {
                            if(CamionesD[j].carga[k].BC[l].getProduct(Prod) == id_producto)
                                sum += CamionesD[j].carga[k].BC[l].ocupBunch(Prod);
                        }
                    }
                }
            }
           
        }
    }       // end checking CamionesD
    // camiones 2 que llegan a destino final sucursal
    if((Camiones2.size() > 0) && (Nodes[dnode].get_type() == 2))
    {
        for(unsigned int j=0; j<Camiones2.size(); j++)
        {
            for(unsigned int m=0; m<Camiones2[j].size(); m++)
            {
                if(Camiones2[j][m].carga.size()>0)
                {
                    for(unsigned int k=0; k<Camiones2[j][m].carga.size(); k++)
                    {
                        if(Camiones2[j][m].carga[k].BC.size() > 0)
                        {
                            for(unsigned int l=0; l<Camiones2[j][m].carga[k].BC.size(); l++)
                            {
                                if(Camiones2[j][m].carga[k].BC[l].getProduct(Prod) == id_producto)
                                    sum += Camiones2[j][m].carga[k].BC[l].ocupBunch(Prod);
                            }
                        }
                    }
                }       // end for k
            }           // end for m
        }   // end for j
    }   // end checking Camiones2
    
    // camiones S que llegan a destino final sucursal
    if(CamionesS.size() > 0)
    {
        if((Nodes[unode].get_type() == 2)&&(Nodes[dnode].get_type() == 2))  //arco S-S
        {
            for(unsigned int j=0; j<CamionesS.size(); j++)
            {
                for(unsigned int m=0; m<CamionesS[j].size(); m++)
                {
                    if(CamionesS[j][m].carga.size()>0)
                    {
                        for(unsigned int k=0; k<CamionesS[j][m].carga.size(); k++)
                        {
                            if(CamionesS[j][m].carga[k].BC.size() > 0)
                            {
                                for(unsigned int l=0; l<CamionesS[j][m].carga[k].BC.size(); l++)
                                {
                                    if(CamionesS[j][m].carga[k].BC[l].getProduct(Prod) == id_producto)
                                        sum += CamionesS[j][m].carga[k].BC[l].ocupBunch(Prod);
                                }
                            }
                        }   // end for k
                    }
                }           // end for m
            }   // end for j
        }   // end if arcos S-S (se bajan todos los que quedan
        else if((Nodes[unode].get_type() == 1)&&(Nodes[dnode].get_type() == 2))  //arco P-S
        {
            for(unsigned int j=0; j<CamionesS.size(); j++)
            {
                for(unsigned int m=0; m<CamionesS[j].size(); m++)
                {
                    if(CamionesS[j][m].carga.size()>0)
                    {
                        for(unsigned int k=0; k<CamionesS[j][m].carga.size(); k++)
                        {
                            if((CamionesS[j][m].carga[k].BC.size() > 0)&&(CamionesS[j][m].carga[k].get_descarga()==1))  //se bajan los descarga 1
                            {
                                for(unsigned int l=0; l<CamionesS[j][m].carga[k].BC.size(); l++)
                                {
                                    if(CamionesS[j][m].carga[k].BC[l].getProduct(Prod) == id_producto)
                                        sum += CamionesS[j][m].carga[k].BC[l].ocupBunch(Prod);
                                }
                            }
                        }   // end for k
                    }
                }           // end for m
            }   // end for j
        }       // end if arcos P-S (se bajan los descarga 1)
        
    }   // end para CamionesS
    
    return(sum);
}


//cuenta número de cajas de un cierto producto id_producto llegando a nodo fin de este edge
double Edge::Cajas_producto(string id_producto, vector<Node>& Nodes, vector<Producto>& Prod)
{
    double sum = 0;
    //primero chequeo camiones directos
    if(CamionesD.size()>0)
    {
        for(unsigned int j=0; j<CamionesD.size(); j++)
        {
            if(CamionesD[j].carga.size()>0)
            {
                for(unsigned int k=0; k<CamionesD[j].carga.size(); k++)
                {
                    if(CamionesD[j].carga[k].BC.size() > 0)
                    {
                        for(unsigned int l=0; l<CamionesD[j].carga[k].BC.size(); l++)
                        {
                            if(CamionesD[j].carga[k].BC[l].getProduct(Prod) == id_producto)
                                sum += CamionesD[j].carga[k].BC[l].getCajas();
                        }
                    }
                }
            }
           
        }
    }       // end checking CamionesD
    // camiones 2 que llegan a destino final sucursal
    if((Camiones2.size() > 0) && (Nodes[dnode].get_type() == 2))
    {
        if(Camiones2.size()>0)
        {
            for(unsigned int j=0; j<Camiones2.size(); j++)
            {
                for(unsigned int m=0; m<Camiones2[j].size(); m++)
                {
                    if(Camiones2[j][m].carga.size()>0)
                    {
                        for(unsigned int k=0; k<Camiones2[j][m].carga.size(); k++)
                        {
                            if(Camiones2[j][m].carga[k].BC.size() > 0)
                            {
                                for(unsigned int l=0; l<Camiones2[j][m].carga[k].BC.size(); l++)
                                {
                                    if(Camiones2[j][m].carga[k].BC[l].getProduct(Prod) == id_producto)
                                        sum += Camiones2[j][m].carga[k].BC[l].getCajas();
                                }
                            }
                        }   // end for k
                    }
                }           // end for m
            }   // end for j
        }
    }   // end checking Camiones2
    
    // camiones S que llegan a destino final sucursal
    // camiones S que llegan a destino final sucursal
    if(CamionesS.size() > 0)
    {
        if((Nodes[unode].get_type() == 2)&&(Nodes[dnode].get_type() == 2))  //arco S-S
        {
            for(unsigned int j=0; j<CamionesS.size(); j++)
            {
                for(unsigned int m=0; m<CamionesS[j].size(); m++)
                {
                    if(CamionesS[j][m].carga.size()>0)
                    {
                        for(unsigned int k=0; k<CamionesS[j][m].carga.size(); k++)
                        {
                            if(CamionesS[j][m].carga[k].BC.size() > 0)
                            {
                                for(unsigned int l=0; l<CamionesS[j][m].carga[k].BC.size(); l++)
                                {
                                    if(CamionesS[j][m].carga[k].BC[l].getProduct(Prod) == id_producto)
                                        sum += CamionesS[j][m].carga[k].BC[l].getCajas();
                                }
                            }
                        }   // end for k
                    }
                }           // end for m
            }   // end for j
        }   // end if arcos S-S (se bajan todos los que quedan
        else if((Nodes[unode].get_type() == 1)&&(Nodes[dnode].get_type() == 2))  //arco P-S
        {
            for(unsigned int j=0; j<CamionesS.size(); j++)
            {
                for(unsigned int m=0; m<CamionesS[j].size(); m++)
                {
                    if(CamionesS[j][m].carga.size()>0)
                    {
                        for(unsigned int k=0; k<CamionesS[j][m].carga.size(); k++)
                        {
                            if((CamionesS[j][m].carga[k].BC.size() > 0)&&(CamionesS[j][m].carga[k].get_descarga()==1))  //se bajan los descarga 1
                            {
                                for(unsigned int l=0; l<CamionesS[j][m].carga[k].BC.size(); l++)
                                {
                                    if(CamionesS[j][m].carga[k].BC[l].getProduct(Prod) == id_producto)
                                        sum += CamionesS[j][m].carga[k].BC[l].getCajas();
                                }
                            }
                        }   // end for k
                    }
                }           // end for m
            }   // end for j
        }       // end if arcos P-S (se bajan los descarga 1)
        
    }   // end para CamionesS
    
    return(sum);
}

//mejora para poder cargar pallet no cargado. Sólo aplica sobre camiones en camiones directos P-P, y tb debería servir para pirimer tramo de CamionesS
void Edge::FixPalletsNoCargadosPP(vector<Camion>& CamAux, vector<Pallet>& PNC, int noptions, vector<Producto>& Prod, vector<Pallet>& sobP, vector<Familia>& FA)
{
    sobP.clear();
    vector<trioPallet> SobranteP;
    SobranteP.clear();
    vector<Pallet> Paux;
    Paux.clear();
    for(unsigned int k=0; k<PNC.size(); k++)
        Paux.push_back(PNC[k]);
    bool Ptipo = FA[PNC[0].familia].congelado;   //acá se identifica si el pallet que no se cargó es congelado o no
    
    for(unsigned int k=0; k<CamAux.size(); k++)
    {
        
        for(unsigned int j=0; j<noptions; j++)
        {
            Pallet pn = RemovePallet(k,CamAux,Ptipo,FA);   //siempre estos pallets serán aceptables de descargar
            
            if(pn.getid_pallet() != -1)
                Paux.push_back(pn);
        }   // end for j
    }   // end for k
    
    //hacemos la agrupación para que los pallets de la misma familia queden juntos
    sort(Paux.begin(),Paux.end(),&CompareFam);
    
    //vuelvo a tratar de cargar, pero no ordeno los pallets que saqué (diversidad)
    //si el que sobraba es congelado, eso implica que no hay fresco y por lo tanto la restriccion de a pares no me importa, tengo que chequear número mínimo de ocngelados, pero si hay puro congelado esa restricción no debería tener problemas tampoco.. Lo más probable es que se me quede fresco abajo.

    if(NoFamilia)
        OrdenaCamionesFamilia(0, CamAux);
    
    for(unsigned int kk=0; kk<Paux.size(); kk++)
    {
        if(!NoFamilia)
        {
            int fam = Paux[kk].familia;
            OrdenaCamionesFamilia(fam, CamAux);
        }
        
        LoadCamionesSingle(CamAux, 0, Paux, kk, Prod, SobranteP, FA);
        
    }       // end for cargando pallets
    if(SobranteP.size() > 0)
    {
        for(unsigned int k=0; k<SobranteP.size(); k++)
            sobP.push_back(Paux[SobranteP[k].idPallet]);
        
        sort(sobP.begin(),sobP.end(),&CompareFam);
    }
   
}   // end fn FixPNC_PP

// sirve para camiones y2
void Edge::FixPalletsNoCargadosPS(vector<Camion>& CamAux, vector<Camion>& CamCons, vector<Pallet>& PNC, int noptions, vector<Producto>& Prod, vector<Pallet>& sobP, vector<Familia>& FA)
{
    sobP.clear();
    vector<trioPallet> SobranteP;
    SobranteP.clear();
    vector<Pallet> Paux;
    Paux.clear();
    
    for(unsigned int k=0; k<PNC.size(); k++)
        Paux.push_back(PNC[k]);
    bool Ptipo = FA[PNC[0].familia].congelado;   //acá se identifica si el pallet que no se cargó es congelado o no
    
    vector<Camion> CamAgregado;     //acá se considera la opción de tener camiones y2
    CamAgregado = CamAux;
    if(CamCons.size() > 0)
        CamAgregado.insert(CamAgregado.end(),CamCons.begin(),CamCons.end());
    
    for(unsigned int k=0; k<CamAgregado.size(); k++)
    {
        
        for(unsigned int j=0; j<noptions; j++)
        {
            Pallet pn = RemovePallet(k,CamAgregado,Ptipo,FA);
            
            if(pn.getid_pallet() != -1)
                Paux.push_back(pn);
        }   // end for j
    }   // end for k
    
    
    //hacemos la agrupación para que los pallets de la misma familia queden juntos
    sort(Paux.begin(),Paux.end(),&CompareFam);

    //vuelvo a tratar de cargar, pero no ordeno los pallets que saqué (diversidad)
    //si el que sobraba es congelado, eso implica que no hay fresco y por lo tanto la restriccion de a pares no me importa, tengo que chequear número mínimo de ocngelados, pero si hay puro congelado esa restricción no debería tener problemas tampoco.. Lo más probable es que se me quede fresco abajo.
//    Paux = sortCPs(Paux, &ComparePeso);
    
    if(NoFamilia)
        OrdenaCamionesFamilia(0, CamAgregado);
    
    for(unsigned int kk=0; kk<Paux.size(); kk++)
    {
        if(!NoFamilia)
        {
            int fam = Paux[kk].familia;
            OrdenaCamionesFamilia(fam, CamAgregado);
        }

        LoadCamionesSingle(CamAgregado, 0, Paux, kk, Prod, SobranteP, FA);
        
    }       // end for cargando pallets
        
    if(SobranteP.size() > 0)
    {
        for(unsigned int k=0; k<SobranteP.size(); k++)
            sobP.push_back(Paux[SobranteP[k].idPallet]);
        
        sort(sobP.begin(),sobP.end(),&CompareFam);
    }
    
    //separa los vectores de camiones
    separa_camiones(CamAgregado, CamAux, CamCons);
   
}   // end fn FixPNC_PS

Pallet Edge::RemovePallet(int idCam, vector<Camion>& CamAux, bool congelado, vector<Familia>& FA)
{
    srand((unsigned)time(NULL));
    vector<int> idTipo;
    idTipo.clear();
    
    for(unsigned int m=0; m<CamAux[idCam].carga.size(); m++)
    {
        if((FA[CamAux[idCam].carga[m].familia].congelado == congelado) && (CamAux[idCam].carga[m].get_prev_edge() == -1))
            idTipo.push_back(m);
    }
    size_t NN = idTipo.size();
    if(NN > 0)
    {
        int Rnumber = rand() % NN;  // si NN = 3, genera RV 0,1,2
        auto elem_to_remove = CamAux[idCam].carga.begin() + idTipo[Rnumber];
        if(elem_to_remove != CamAux[idCam].carga.end())
        {
            Pallet PRem = CamAux[idCam].carga[idTipo[Rnumber]];
            int familia = CamAux[idCam].carga[idTipo[Rnumber]].familia;
            CamAux[idCam].carga.erase(elem_to_remove);
            CamAux[idCam].PalletsPorFamilia[familia]--;
            CamAux[idCam].computePesoOcupacion();
            return(PRem);
        }
        else
            return(Pallet(-1,-1,-1,-1,-1));
    }
    else
        return(Pallet(-1,-1,-1,-1,-1));
}       // end RemovePallet

void DeleteAll(vector<Pallet>& data, const vector<int>& deleteIndices)
{
    vector<bool> markedElements(data.size(), false);
    vector<Pallet> tempBuffer;
    tempBuffer.reserve(data.size()-deleteIndices.size());

    for (vector<int>::const_iterator itDel = deleteIndices.begin(); itDel != deleteIndices.end(); itDel++)
        markedElements[*itDel] = true;

    for (size_t i=0; i<data.size(); i++)
    {
        if (!markedElements[i])
            tempBuffer.push_back(data[i]);
    }
    data = tempBuffer;
}

// fn para transferir dos pallets congelados de un camión a otro, siempre que sea factible. Prueba combinaciones random de a pares hasta que puede hacer el traspaso
bool Edge::MovePalletC(int idCamO, int idCamD, vector<Camion>& CamAux, vector<Producto>& Prod)
{
    srand((unsigned)time(NULL));
    size_t NN = CamAux[idCamO].carga.size();
    size_t NN2 = CamAux[idCamD].carga.size();
    
    if(NN >= 2)     // solo puedo sacarle carga a los camiones que tienen pallets para sacar
    {
        for(unsigned int m=0; m<MaxIterAjuste; m++)
        {
            int Rnumber1 = 0, Rnumber2 = 0;
            //tratemos de que los pallets que se intercambien sean de la misma familia
            for(unsigned int pp=0; pp<MAXIterFam; pp++)
            {
                Rnumber1 = rand() % NN;  // si NN = 3, genera RV 0,1,2
                Rnumber2 = rand() % NN;
                while(Rnumber1 == Rnumber2)
                    Rnumber2 = rand() % NN;
               
                int familia1 = CamAux[idCamO].carga[Rnumber1].familia;
                int familia2 = CamAux[idCamO].carga[Rnumber2].familia;
                if(familia1 == familia2)
                    break;
            }
            
            double PesoExtra = CamAux[idCamO].carga[Rnumber1].peso + CamAux[idCamO].carga[Rnumber2].peso;
            bool RestrP = CamAux[idCamD].sobreCargaP(2);
            bool RestrK = CamAux[idCamD].sobreCargaK(PesoExtra, Prod);

            if((!RestrP)&&(!RestrK))    //traspaso factible de id0 a id1
            {
                Pallet PRem1 = CamAux[idCamO].carga[Rnumber1];
                int familia1 = CamAux[idCamO].carga[Rnumber1].familia;
                Pallet PRem2 = CamAux[idCamO].carga[Rnumber2];
                int familia2 = CamAux[idCamO].carga[Rnumber2].familia;
                
                vector<int> indicesBorrar = {Rnumber1,Rnumber2};
                DeleteAll(CamAux[idCamO].carga, indicesBorrar);
                CamAux[idCamD].carga.push_back(PRem1);
                CamAux[idCamD].carga.push_back(PRem2);
                    
                CamAux[idCamO].PalletsPorFamilia[familia1]--;
                CamAux[idCamO].PalletsPorFamilia[familia2]--;
                CamAux[idCamD].PalletsPorFamilia[familia1]++;
                CamAux[idCamD].PalletsPorFamilia[familia2]++;
                
                CamAux[idCamO].computePesoOcupacion();
                CamAux[idCamD].computePesoOcupacion();
                
                return(true);
            }       // end if codición factible
        
        }   // end for
    }       // end if NN>0
    return(false);
}       // end MOvePalletC

//fn que encuentra el camión factible con carga más cercana a su Máximo de pallets, por debajo, para ir con carga mixta. Ideal es carga = Max Pallets
int FindCamionMax(vector<Camion>& CamAux)
{
    int DeltaCarga = 100;       //se parte con un número grande
    int ichosen = -1;
    for(unsigned int j=0; j<CamAux.size(); j++)
    {
        int delta = CamAux[j].maxPalletsC - (int)CamAux[j].carga.size();
        if((delta >= 0) && (delta < DeltaCarga))
        {
            DeltaCarga = delta;
            ichosen = j;
        }
    }
    return(ichosen);
}

//fn que encuentra el camión que tiene carga de congelados positiva pero no está a tope (CTC), así como algún camion que si esté a tope si lo hay (CMC)
void FindCamionMax2(int* CMC, int* CTC, vector<Camion>& CamAux)
{
    int ichosenM = -1;
    int ichosenT = -1;
    
    for(unsigned int j=0; j<CamAux.size(); j++)
    {
        if(CamAux[j].carga.size() == CamAux[j].capacP)
            ichosenM = j;
        else if((CamAux[j].carga.size() < CamAux[j].capacP)&&(CamAux[j].carga.size()>0))
            ichosenT = j;
    }
    *CMC = ichosenM;
    *CTC = ichosenT;
}

// es true si todos los camiones están OK con las restricciones de congleado, false si alguno falla
bool CheckCamionesCongelados(vector<Camion>& CamAux)
{
    int count = 0;
    for(unsigned int i=0; i<CamAux.size(); i++)
    {
        if(((CamAux[i].carga.size() >= CamAux[i].minPalletsC) && (CamAux[i].carga.size() <= CamAux[i].maxPalletsC))||(CamAux[i].carga.size() == 0) || (CamAux[i].carga.size() == CamAux[i].capacP))
            count ++;
    }
    if(count == CamAux.size())
        return(true);
    else
        return(false);
}   // end fn Check

//esta función ajusta los pallets para cumplir con restricción de congelados
bool Edge::AjustaCamiones(vector<Camion>& CamAux, vector<Producto>& Prod)
{
    if(CheckCamionesCongelados(CamAux))
       return(true);

    for(unsigned int i=0; i<CamAux.size(); i++)
    {
        int j = FindCamionMax(CamAux);
        
        if((j!=i)&&(j!=-1))
        {
            
            if((CamAux[i].carga.size() < CamAux[i].minPalletsC) && (CamAux[i].carga.size() > 0))
            {
                bool cond1 = CamAux[j].carga.size() > (CamAux[j].minPalletsC + 2);
                
                if(cond1)
                {
                    bool transfer = MovePalletC(j,i,CamAux,Prod);
                    if(CheckCamionesCongelados(CamAux))
                        return(true);
                    
                    while((CamAux[i].carga.size() < CamAux[i].minPalletsC)&&(transfer)&&(cond1))
                    {
                        transfer = MovePalletC(j,i,CamAux,Prod);
                        if(CheckCamionesCongelados(CamAux))
                            return(true);
                    }
                }
                else
                {
                    bool transfer = MovePalletC(i,j,CamAux,Prod);   //dejo vacío el camión i
                    if(CheckCamionesCongelados(CamAux))
                        return(true);
                }
            }       // en if camiones que tienen menos que el mínimo
            
            else if((CamAux[i].carga.size() > CamAux[i].maxPalletsC) && (CamAux[i].carga.size() < CamAux[i].capacP))
            {
                bool cond2 = CamAux[j].carga.size() > (CamAux[j].minPalletsC + 2);
                
                if(cond2)
                {
                    bool transfer = MovePalletC(j,i,CamAux,Prod);
                    if(CheckCamionesCongelados(CamAux))
                        return(true);
                    
                    while((CamAux[i].carga.size() < CamAux[i].capacP)&&(transfer)&&(cond2))
                    {
                        transfer = MovePalletC(j,i,CamAux,Prod);
                        if(CheckCamionesCongelados(CamAux))
                            return(true);
                    }
                }
                else
                {
                    bool transfer = MovePalletC(i,j,CamAux,Prod);
                    if(CheckCamionesCongelados(CamAux))
                        return(true);
                    
                    while((CamAux[i].carga.size() < CamAux[i].capacP)&&(transfer))
                    {
                        transfer = MovePalletC(i,j,CamAux,Prod);
                        if(CheckCamionesCongelados(CamAux))
                            return(true);
        
                    }
                }
                
            }   // end else if camiones que tienen mas que el máximo
        }  // end if chequeando que sean camiones distintos
    }       // end for chequeando y ajustando camiones en CamAux
    
    if(CheckCamionesCongelados(CamAux))
        return(true);
    else
       return(false);
    
}   // end fn AjustaCamiones

//esta función ajusta los pallets para cumplir con restricción de congelados, caso de ir llenando camiones a tope
bool Edge::AjustaCamiones2(vector<Camion>& CamAux, vector<Producto>& Prod)
{
    if(CheckCamionesCongelados(CamAux))
       return(true);
    
    int camT, camM;
    FindCamionMax2(&camM, &camT, CamAux);
    
    if(camT != -1)
    {
        if(CamAux[camT].carga.size() < CamAux[camT].minPalletsC)    //hay que corregir, y si pasa esto necesariamente camM != -1
        {
            if(camM == -1)  //camión está solo y no hay forma que cumpla
            {
//              cout << "Error en AjustaCamiones2 con camión : " << CamAux[camT].get_idc() << endl;
                return(false);
//                exit(1);
            }
            else{
                bool cond = true;
                while(cond)
                {
                    bool transfer = MovePalletC(camM,camT,CamAux,Prod);
                    if(CheckCamionesCongelados(CamAux))
                        return(true);
                }
            }
        }   // end if
        else if(CamAux[camT].carga.size() > CamAux[camT].maxPalletsC)
        {
            int target0 = -1;
            for(unsigned int pp=0; pp<CamAux.size(); pp++)
            {
                if(CamAux[pp].carga.size() == 0)
                {
                    target0 = pp;
                    break;
                }
            }
            if(target0 == -1)   //no se puede ajustar estos congelados
                return(false);
            else
            {
                bool cond = true;
                while(cond)
                {
                    bool transfer = MovePalletC(camT,target0,CamAux,Prod);
                    if(CheckCamionesCongelados(CamAux))
                        return(true);
                }
            }
        }
    }
    
    if(CheckCamionesCongelados(CamAux))
        return(true);
    else
       return(false);

}   // end fn AjustaCamiones2

void Edge::AjustaTV(vector<Node>& Nodes)    //ajusta tiempos de viaje en base a distancia haversine
{
    if(tViaje <= 0)
    {
        double dist = Nodes[unode].Dist(Nodes[dnode]);
        if(dist<= 12000)
            tViaje = 1;
        else if((dist > 12000)&&(dist <= 15000))
            tViaje = 2;
        else if((dist > 15000)&&(dist <= 55000))
            tViaje = 3;
        else if((dist > 55000)&&(dist <= 90000))
            tViaje = 4;
        else if((dist > 90000)&&(dist <= 160000))
            tViaje = 5;
        else if((dist > 160000)&&(dist <= 250000))
            tViaje = 8;
        else if((dist > 250000)&&(dist <= 350000))
            tViaje = 11;
        else if(dist > 350000)
            tViaje = 30;
    }
}

//hace el match entre sectores y estados de detalle para Sap, con el detalle para Capacidades
void MapEstadoSector(int vecID, string* estado, string* sector)
{
    if(vecID == 0)
    {
        *estado = "Congelado";
        *sector = "Pollo";
    }
    else if(vecID == 1)
    {
        *estado = "Congelado";
        *sector = "Cerdo";
    }
    else if(vecID == 2)
    {
        *estado = "Congelado";
        *sector = "Elaborado";
    }
    else if(vecID == 3)
    {
        *estado = "Congelado";
        *sector = "Salmón";
    }
    else if(vecID == 4)
    {
        *estado = "Congelado";
        *sector = "Pavo";
    }
    else if(vecID == 5)
    {
        *estado = "Congelado";
        *sector = "Hortalizas y Frutas";
    }
    else if(vecID == 6)
    {
        *estado = "Fresco";
        *sector = "Pollo";
    }
    else if(vecID == 7)
    {
        *estado = "Fresco";
        *sector = "Cerdo";
    }
    else if(vecID == 8)
    {
        *estado = "Fresco";
        *sector = "Pavo";
    }
    else if(vecID == 9)
    {
        *estado = "Fresco";
        *sector = "Cecina";
    }
    else if(vecID == 10)
    {
        *estado = "Fresco";
        *sector = "Salmón";
    }
}

//identifica los pallets que no vienen de un arco previo
vector<Pallet> DeltaDemY2(Camion& Cam2, vector <Node>& Nodes)
{
    vector<Pallet> DeltaD;
    DeltaD.clear();
    for(unsigned int k=0; k<Cam2.carga.size();k++)
    {
        if(Cam2.carga[k].get_prev_edge() == -1)
            DeltaD.push_back(Cam2.carga[k]);
    }
    return(DeltaD);
}       // end fn DeltaDemanda

vector<InfoCapac> Edge::DetalleAnalisisCapacidad(bool carga, vector<Node>& Nodes, vector<Producto>& Prod, vector<Familia>& FA)
{
    vector<InfoCapac> DataCruce;
    DataCruce.clear();
    if(carga)       //corresponde a un proceso de carga en nodo origen
    {
        if(Nodes[v()].get_type() == 1)  //corresponde analizar proceso de carga
        {
            //para camiones directos todo se carga en el origen
            for(unsigned int i=0; i<CamionesD.size(); i++)
            {
                CamionesD[i].computePesoOcupacion();
                vector<double> KPS = CamionesD[i].KilosPorSector(CamionesD[i].carga,FA, Prod);
                vector<double> PPS = CamionesD[i].palletsPorSector(CamionesD[i].carga, FA, Prod);
                for(unsigned k=0; k<KPS.size(); k++)
                {
                    if(KPS[k] > 0)  //caso de reportar un elemento a agregar
                    {
                        string estadoF, sectorF;
                        MapEstadoSector(k, &estadoF, &sectorF);
                        DataCruce.push_back(InfoCapac(CamionesD[i].get_idc(),CamionesD[i].get_stype(),CamionesD[i].ClasifCamion,Nodes[v()].get_cod(),"carga",estadoF,sectorF,CamionesD[i].separador(FA),false,PPS[k],KPS[k]));
                    }
                }
                
            }   //end chequeando camiones directos
            for(unsigned int j=0; j<Camiones2.size(); j++)
            {
                for(unsigned int k=0; k<Camiones2[j].size(); k++)
                {
                    if(Camiones2[j][k].get_prev() == -1)    //estoy en la primera planta del viaje P-P-S, todo se carga
                    {
                        Camiones2[j][k].computePesoOcupacion();
                        vector<double> KPS = Camiones2[j][k].KilosPorSector(Camiones2[j][k].carga, FA, Prod);
                        vector<double> PPS = Camiones2[j][k].palletsPorSector(Camiones2[j][k].carga, FA, Prod);
                        for(unsigned m=0; m<KPS.size(); m++)
                        {
                            if(KPS[m] > 0)  //caso de reportar un elemento a agregar
                            {
                                string estadoF, sectorF;
                                MapEstadoSector(m, &estadoF, &sectorF);
                                DataCruce.push_back(InfoCapac(Camiones2[j][k].get_idc(),Camiones2[j][k].get_stype(),Camiones2[j][k].ClasifCamion,Nodes[v()].get_cod(),"carga",estadoF,sectorF,Camiones2[j][k].separador(FA),false,PPS[m],KPS[m]));
                            }
                        }
                    }       // end if mirando planta 1
                    else    //acá sólo se reporta la carga adicional que se carga en la segunda planta
                    {
                        Camiones2[j][k].computePesoOcupacion();
                        vector<Pallet> ExtraPallets = DeltaDemY2(Camiones2[j][k], Nodes);           //extra pallets que se cargan en nodo intermedio
                        vector<double> KPS = Camiones2[j][k].KilosPorSector(ExtraPallets, FA, Prod);
                        vector<double> PPS = Camiones2[j][k].palletsPorSector(ExtraPallets, FA, Prod);
                        for(unsigned m=0; m<KPS.size(); m++)
                        {
                            if(KPS[m] > 0)  //caso de reportar un elemento a agregar
                            {
                                string estadoF, sectorF;
                                MapEstadoSector(m, &estadoF, &sectorF);
                                DataCruce.push_back(InfoCapac(Camiones2[j][k].get_idc(),Camiones2[j][k].get_stype(),Camiones2[j][k].ClasifCamion,Nodes[v()].get_cod(),"carga",estadoF,sectorF,Camiones2[j][k].separador(FA),false,PPS[m],KPS[m]));
                            }
                        }
                    
                    }   // end else estando en planta 2
                }
            }       // end for Camiones 2
            
            for(unsigned int j=0; j<CamionesS.size(); j++)
            {
                for(unsigned int k=0; k<CamionesS[j].size(); k++)
                {
                    CamionesS[j][k].computePesoOcupacion();
                    vector<double> KPS = CamionesS[j][k].KilosPorSector(CamionesS[j][k].carga, FA, Prod);
                    vector<double> PPS = CamionesS[j][k].palletsPorSector(CamionesS[j][k].carga, FA, Prod);
                    for(unsigned m=0; m<KPS.size(); m++)
                    {
                        if(KPS[m] > 0)  //caso de reportar un elemento a agregar
                        {
                            string estadoF, sectorF;
                            MapEstadoSector(m, &estadoF, &sectorF);
                            DataCruce.push_back(InfoCapac(CamionesS[j][k].get_idc(),CamionesS[j][k].get_stype(),CamionesS[j][k].ClasifCamion,Nodes[v()].get_cod(),"carga",estadoF,sectorF,CamionesS[j][k].separador(FA),true,PPS[m],KPS[m]));
                        }
                    }
                    
                }   // end for
                
            }       // end for Camiones S
            
        }   //revisando nodo origen que sea planta
    }
    else        //proceso de descarga en nodo destino
    {
        if(Nodes[w()].get_type() == 2)  //corresponde analizar proceso de descarga
        {
            //para camiones directos todo se descarga en el destino
            for(unsigned int i=0; i<CamionesD.size(); i++)
            {
                CamionesD[i].computePesoOcupacion();
                vector<double> KPS = CamionesD[i].KilosPorSector(CamionesD[i].carga,FA, Prod);
                vector<double> PPS = CamionesD[i].palletsPorSector(CamionesD[i].carga, FA, Prod);
                for(unsigned k=0; k<KPS.size(); k++)
                {
                    if(KPS[k] > 0)  //caso de reportar un elemento a agregar
                    {
                        string estadoF, sectorF;
                        MapEstadoSector(k, &estadoF, &sectorF);
                        DataCruce.push_back(InfoCapac(CamionesD[i].get_idc(),CamionesD[i].get_stype(),CamionesD[i].ClasifCamion,Nodes[w()].get_cod(),"descarga",estadoF,sectorF,CamionesD[i].separador(FA),false,PPS[k],KPS[k]));
                    }
                }
                
            }   //end chequeando camiones directos
            for(unsigned int j=0; j<Camiones2.size(); j++)
            {
                for(unsigned int k=0; k<Camiones2[j].size(); k++)
                {
                    Camiones2[j][k].computePesoOcupacion();
                    vector<double> KPS = Camiones2[j][k].KilosPorSector(Camiones2[j][k].carga, FA, Prod);
                    vector<double> PPS = Camiones2[j][k].palletsPorSector(Camiones2[j][k].carga, FA, Prod);
                    for(unsigned m=0; m<KPS.size(); m++)
                    {
                        if(KPS[m] > 0)  //caso de reportar un elemento a agregar
                        {
                            string estadoF, sectorF;
                            MapEstadoSector(m, &estadoF, &sectorF);
                            DataCruce.push_back(InfoCapac(Camiones2[j][k].get_idc(),Camiones2[j][k].get_stype(),Camiones2[j][k].ClasifCamion,Nodes[w()].get_cod(),"descarga",estadoF,sectorF,Camiones2[j][k].separador(FA),false,PPS[m],KPS[m]));
                        }
                    }
                }
 
            }       // end for Camiones 2
            
            for(unsigned int j=0; j<CamionesS.size(); j++)
            {
                for(unsigned int k=0; k<CamionesS[j].size(); k++)
                {
                    
                    if(Nodes[v()].get_type() == 2)  //caso simple, w es el último punto de descarga, todo se baja
                    {
                        CamionesS[j][k].computePesoOcupacion();
                        vector<double> KPS = CamionesS[j][k].KilosPorSector(CamionesS[j][k].carga, FA, Prod);
                        vector<double> PPS = CamionesS[j][k].palletsPorSector(CamionesS[j][k].carga, FA, Prod);
                        for(unsigned m=0; m<KPS.size(); m++)
                        {
                            if(KPS[m] > 0)  //caso de reportar un elemento a agregar
                            {
                                string estadoF, sectorF;
                                MapEstadoSector(m, &estadoF, &sectorF);
                                DataCruce.push_back(InfoCapac(CamionesS[j][k].get_idc(),CamionesS[j][k].get_stype(),CamionesS[j][k].ClasifCamion,Nodes[w()].get_cod(),"descarga",estadoF,sectorF,CamionesS[j][k].separador(FA),true,PPS[m],KPS[m]));
                            }
                        }
                    }
                    else    //sucursal es intemedia, así que sólo se descarga parte de la carga
                    {
                        vector<Pallet> AuxSuc1;     //acá se guardan los que se descargan en primera sucursal en P-S-S
                        AuxSuc1.clear();
                        for(unsigned int p=0; p<CamionesS[j][k].carga.size(); p++)
                        {
                            if(CamionesS[j][k].carga[p].get_descarga() == 1)
                                AuxSuc1.push_back(CamionesS[j][k].carga[p]);
                        }
                        
                        CamionesS[j][k].computePesoOcupacion();
                        vector<double> KPS = CamionesS[j][k].KilosPorSector(AuxSuc1, FA, Prod);
                        vector<double> PPS = CamionesS[j][k].palletsPorSector(AuxSuc1, FA, Prod);
                        for(unsigned m=0; m<KPS.size(); m++)
                        {
                            if(KPS[m] > 0)  //caso de reportar un elemento a agregar
                            {
                                string estadoF, sectorF;
                                MapEstadoSector(m, &estadoF, &sectorF);
                                DataCruce.push_back(InfoCapac(CamionesS[j][k].get_idc(),CamionesS[j][k].get_stype(),CamionesS[j][k].ClasifCamion,Nodes[w()].get_cod(),"descarga",estadoF,sectorF,CamionesS[j][k].separador(FA),true,PPS[m],KPS[m]));
                            }
                        }
                        
                    }       // end else caso descarga en sucursal 1
                
                }   // end for
                
            }       // end for Camiones S
            
        }   //end if revisando nodo destino sea una sucursal
        else if (Nodes[w()].get_type() == 1)    //voy a ver las descargas de los camiones interplanta
        {
            //sólo camiones directos interplanta descargando en destino que es planta
            for(unsigned int i=0; i<CamionesD.size(); i++)
            {
                CamionesD[i].computePesoOcupacion();
                vector<double> KPS = CamionesD[i].KilosPorSector(CamionesD[i].carga,FA, Prod);
                vector<double> PPS = CamionesD[i].palletsPorSector(CamionesD[i].carga, FA, Prod);
                for(unsigned k=0; k<KPS.size(); k++)
                {
                    if(KPS[k] > 0)  //caso de reportar un elemento a agregar
                    {
                        string estadoF, sectorF;
                        MapEstadoSector(k, &estadoF, &sectorF);
                        DataCruce.push_back(InfoCapac(CamionesD[i].get_idc(),CamionesD[i].get_stype(),CamionesD[i].ClasifCamion,Nodes[w()].get_cod(),"descarga",estadoF,sectorF,CamionesD[i].separador(FA),false,PPS[k],KPS[k]));
                    }
                }
                
            }   //end chequeando camiones directos
            
        }   //end viendo descargas de interplanta
    
    }   // end else revisando descarga
    return(DataCruce);
}

string Edge::PrintDetails(string IDEN, vector<Camion>& CamionesP, vector<vector<Pallet>>& CargaP, vector<Node>& Nodes, vector<Producto>& Prod)
{
    stringstream s;
    s << "E:" << edge_id << "  NO: " << Nodes[unode].get_cod() << " ND: " << Nodes[dnode].get_cod() << " xsize: " << x.size() << " x2size: " << x2.size() << " xrsize: " << xr.size() << " xssize: " << xs.size() << " ysize: " << y.size() <<  " y2size: " << y2.size() << " yssize: " << ys.size() << " psize: " << p.size() << " p2size: " << p2.size() << " prsize: " << pr.size() << " zsize: " << z.size() << " z2size: " << z2.size() << " zrsize: " << zr.size() << " zssize: " << zs.size() << endl;
    s << IDEN << endl;
   
    if(CargaP.size() > 0)
    {
        for(unsigned int k=0; k<5;k++)
        {
            s << "Productos familia " << k+1 << endl;
            for(unsigned int i=0; i<CargaP[k].size();i++)
                s << CargaP[k][i].Print_pallet(Prod);
        }
        s << endl;
        s << endl;
    }
    
    s << "Detalle camiones" << endl;
    for(unsigned int i=0; i<CamionesP.size();i++)
        s << CamionesP[i].print_camion() << endl;
    
    s << "Peso total: " << sumUsoX(x,Prod) + sumUsoX(x2,Prod) + sumUsoX(xr,Prod) + sumUsoX(xs,Prod) << endl;
    s << endl;
    s << endl;
         
    return s.str();
}

string Edge::PrintDetailsD(string IDEN, vector<vector<Camion>>& CamionesP, vector<vector<vector<Pallet>>>& CargaP, vector<Node>& Nodes, vector<Producto>& Prod)
{
    stringstream s;
    s << "E:" << edge_id << "  NO: " << Nodes[unode].get_cod() << " ND: " << Nodes[dnode].get_cod() << " xsize: " << x.size() << " x2size: " << x2.size() << " xrsize: " << xr.size() << " xssize: " << xs.size() << " ysize: " << y.size() <<  " y2size: " << y2.size() << " yssize: " << ys.size() << " psize: " << p.size() << " p2size: " << p2.size() << " prsize: " << pr.size() << " zsize: " << z.size() << " z2size: " << z2.size() << " zrsize: " << zr.size() << " zssize: " << zs.size() << endl;
    s << IDEN << endl;
   
    if(CargaP.size() > 0)
    {
        for(unsigned int l=0; l<CargaP.size(); l++)
        {
            if(CargaP[l].size()>0)
            {
                for(unsigned int k=0; k<5;k++)
                {
                    if(CargaP[l][k].size()>0)
                    {
                        s << "Productos familia " << k+1 << endl;
                        
                             for(unsigned int i=0; i<CargaP[l][k].size();i++)
                                 s << CargaP[l][k][i].Print_pallet(Prod);
                    }
                }
            }
            
        }
        s << endl;
    }
    
    if(CamionesP.size() > 0)
    {
        for(unsigned int i=0; i<CamionesP.size();i++)
        {
            if(CamionesP[i].size() > 0)
            {
                for(unsigned int j=0; j<CamionesP[i].size(); j++)
                    s << CamionesP[i][j].print_camion() << endl;
            }
        }
    }
    s << "Peso total: " << sumUsoX(x,Prod) + sumUsoX(x2,Prod) + sumUsoX(xr,Prod) + sumUsoX(xs,Prod) << endl;
    s << endl;
    s << endl;
         
    return s.str();
}

string Edge::PrintDetailsD2(string IDEN, vector<Camion>& CamionesP, vector<vector<vector<Pallet>>>& CargaP, vector<Node>& Nodes, vector<Producto>& Prod)
{
    stringstream s;
    s << "E:" << edge_id << "  NO: " << Nodes[unode].get_cod() << " ND: " << Nodes[dnode].get_cod() << " xsize: " << x.size() << " x2size: " << x2.size() << " xrsize: " << xr.size() << " xssize: " << xs.size() << " ysize: " << y.size() <<  " y2size: " << y2.size() << " yssize: " << ys.size() << " psize: " << p.size() << " p2size: " << p2.size() << " prsize: " << pr.size() << " zsize: " << z.size() << " z2size: " << z2.size() << " zrsize: " << zr.size() << " zssize: " << zs.size() << endl;
    s << IDEN << endl;
   
    if(CargaP.size() > 0)
    {
        for(unsigned int l=0; l<CargaP.size(); l++)
        {
            if(CargaP[l].size()>0)
            {
                for(unsigned int k=0; k<5;k++)
                {
                    if(CargaP[l][k].size()>0)
                    {
                        s << "Productos familia " << k+1 << endl;
                        
                             for(unsigned int i=0; i<CargaP[l][k].size();i++)
                                 s << CargaP[l][k][i].Print_pallet(Prod);
                    }
                }
            }
            
        }
        s << endl;
    }
    
     s << "Detalle camiones" << endl;
       for(unsigned int i=0; i<CamionesP.size();i++)
           s << CamionesP[i].print_camion() << endl;
    s << endl;
    s << "Peso total: " << sumUsoX(x,Prod) + sumUsoX(x2,Prod) + sumUsoX(xr,Prod) + sumUsoX(xs,Prod) << endl;
    s << endl;
    s << endl;
    
    return s.str();
}


// imprime info relevante de vectores x, x2, xr, y, y2, yr, z, z2, zr, etc
string Edge::PrintDetails2(vector<Node>& Nodes, vector<Producto>& Prod)
{
    stringstream s;
    s << "E:" << edge_id << "  NO: " << Nodes[unode].get_cod() << " ND: " << Nodes[dnode].get_cod() << " xsize: " << x.size() << " x2size: " << x2.size() << " xrsize: " << xr.size() << " xssize: " << xs.size() << " ysize: " << y.size() <<  " y2size: " << y2.size() << " yssize: " << ys.size() << " psize: " << p.size() << " p2size: " << p2.size() << " prsize: " << pr.size() << " zsize: " << z.size() << " z2size: " << z2.size() << " zrsize: " << zr.size() << " zssize: " << zs.size() << endl;
    s << endl;
	
    for(unsigned int i=0; i<x.size(); i++)
    {
        int pid = get_Prod_id(x[i].atributo, Prod);
		if (pid != -1)
			s << x[i].print_parts() << ";[" << Prod[pid].get_familia() << "];" << endl;
		else 
			cout << "atributo de X no existe: " << x[i].atributo << endl;
    }
    s << endl;
    
    for(unsigned int i=0; i<x2.size(); i++)
    {
        int pid = get_Prod_id(x2[i].atributo, Prod);
        if (pid != -1)
            s << x2[i].print_parts() << ";[" << Prod[pid].get_familia() << "];" << endl;
        else
            cout << "atributo de X2 no existe: " << x2[i].atributo << endl;
    }
    s << endl;
    
    for(unsigned int i=0; i<xr.size(); i++)
    {
        int pid = get_Prod_id(xr[i].atributo, Prod);
        if (pid != -1)
            s << xr[i].print_parts() << ";[" << Prod[pid].get_familia() << "];" << endl;
        else
            cout << "atributo de Xr no existe: " << xr[i].atributo << endl;
    }
    s << endl;
    
    for(unsigned int i=0; i<xs.size(); i++)
    {
        int pid = get_Prod_id(xs[i].atributo, Prod);
        if (pid != -1)
            s << xs[i].print_parts() << ";[" << Prod[pid].get_familia() << "];" << endl;
        else
            cout << "atributo de Xs no existe: " << xs[i].atributo << endl;
    }
    s << endl;
	
    for(unsigned int i=0; i<z.size(); i++)
        s << z[i].print_parts() << endl;
    s << endl;
    
    for(unsigned int i=0; i<z2.size(); i++)
        s << z2[i].print_parts() << endl;
      s << endl;
    
    for(unsigned int i=0; i<zr.size(); i++)
      s << zr[i].print_parts() << endl;
    s << endl;
    
    for(unsigned int i=0; i<zs.size(); i++)
      s << zs[i].print_parts() << endl;
    s << endl;
	
    for(unsigned int i=0; i<p.size(); i++)
        s << p[i].print_parts() << endl;
    s << endl;
    
    for(unsigned int i=0; i<p2.size(); i++)
        s << p2[i].print_parts() << endl;
    s << endl;
    
    for(unsigned int i=0; i<pr.size(); i++)
        s << pr[i].print_parts() << endl;
    s << endl;
	
    for(unsigned int i=0; i<y.size(); i++)
        s << y[i].print_parts() << endl;
    s << endl;
    
    for(unsigned int i=0; i<y2.size(); i++)
        s << y2[i].print_parts() << endl;
    
    for(unsigned int i=0; i<ys.size(); i++)
        s << ys[i].print_parts() << endl;
	
    return s.str();
}                       // end función que imprime en detalle la red

string Edge::PrintCarga(vector<Node>& Nodes, vector<Producto>& Prod)
{
    stringstream s;
    s << "E:" << edge_id << "  NO: " << Nodes[unode].get_cod() << " ND: " << Nodes[dnode].get_cod();
    s << "Detalle carga directa" << endl;
    if(CargaD.size()>0)
    {
        for(unsigned int i=0; i<CargaD.size(); i++)
            for(unsigned int j=0; j<CargaD[i].size(); j++)
                s << CargaD[i][j].Print_pallet(Prod);
    }
    s << endl;
    s << "Detalle carga consolidado" << endl;
    if(CargaR.size()>0)
    {
        for(unsigned int i=0; i<CargaR.size(); i++)
            for(unsigned int j=0; j<CargaR[i].size(); j++)
                for(unsigned int k=0; k<CargaR[i][j].size(); k++)
                    s << CargaR[i][j][k].Print_pallet(Prod);
    }
    s << endl;
    s << "Detalle carga dos tramos" << endl;
    if(Carga2.size()>0)
    {
        for(unsigned int i=0; i<Carga2.size(); i++)
            for(unsigned int j=0; j<Carga2[i].size(); j++)
                for(unsigned int k=0; k<Carga2[i][j].size(); k++)
                    s << Carga2[i][j][k].Print_pallet(Prod);
    }
    s << "Detalle carga dos tramos viajes compartidos" << endl;
    if(CargaS.size()>0)
    {
        for(unsigned int i=0; i<CargaS.size(); i++)
            for(unsigned int j=0; j<CargaS[i].size(); j++)
                for(unsigned int k=0; k<CargaS[i][j].size(); k++)
                    s << CargaS[i][j][k].Print_pallet(Prod);
    }
    return s.str();
}                   // end PrintCarga

string Edge::PrintCamiones(vector<Node>& Nodes, vector<Producto>& Prod)
{
    stringstream s;
    s << "E:" << edge_id << "  NO: " << Nodes[unode].get_cod() << " ND: " << Nodes[dnode].get_cod();
    
    s << "Detalle camiones directos" << endl;
    if(CamionesD.size()>0)
    {
        for(unsigned int i=0; i<CamionesD.size(); i++)
            s << CamionesD[i].print_camion();
    }
    s << endl;
    s << "Detalle camiones dos tramos" << endl;
    if(Camiones2.size()>0)
    {
        for(unsigned int i=0; i<Camiones2.size(); i++)
            for(unsigned int j=0; j<Camiones2[i].size(); j++)
                s << Camiones2[i][j].print_camion();
    }
    if(CamionesS.size()>0)
    {
        for(unsigned int i=0; i<CamionesS.size(); i++)
            for(unsigned int j=0; j<CamionesS[i].size(); j++)
                s << CamionesS[i][j].print_camionDetails(Prod);
    }
    s << endl;
    return s.str();
}           // end PrintCamiones

string Edge::PrintCamionesDetails(vector<Node>& Nodes, vector<Producto>& Prod)
{
    stringstream s;
    s << "E:" << edge_id << "  NO: " << Nodes[unode].get_cod() << " ND: " << Nodes[dnode].get_cod() << " xsize: " << x.size() << " x2size: " << x2.size() << " xrsize: " << xr.size() << " xssize: " << xs.size() << " ysize: " << y.size() <<  " y2size: " << y2.size() << " yssize: " << ys.size() << " psize: " << p.size() << " p2size: " << p2.size() << " prsize: " << pr.size() << " zsize: " << z.size() << " z2size: " << z2.size() << " zrsize: " << zr.size() << " zssize: " << zs.size() << endl;
    s << endl;
    
    s << "Detalle camiones directos" << endl;
    if(CamionesD.size()>0)
    {
        for(unsigned int i=0; i<CamionesD.size(); i++)
            s << CamionesD[i].print_camionDetails(Prod);
    }
    s << endl;
    s << endl;
    s << "Detalle camiones dos tramos" << endl;
    if(Camiones2.size()>0)
    {
        for(unsigned int i=0; i<Camiones2.size(); i++)
            for(unsigned int j=0; j<Camiones2[i].size(); j++)
                s << Camiones2[i][j].print_camionDetails(Prod);
    }
    s << endl;
    if(CamionesS.size()>0)
    {
        for(unsigned int i=0; i<CamionesS.size(); i++)
            for(unsigned int j=0; j<CamionesS[i].size(); j++)
                s << CamionesS[i][j].print_camionDetails(Prod);
    }
    s << endl;
    return s.str();
}           // end PrintCamiones

string Edge::PrintTVHaversine(vector<Node>& Nodes)
{
    stringstream s;
    double HaverDist = Nodes[unode].Dist(Nodes[dnode]);
    s << edge_id << ";" << Nodes[unode].get_cod() << ";" << Nodes[dnode].get_cod() << ";" << tViaje << ";" << HaverDist << ";";
    return s.str();
}


struct pairCC     // para reportar id de arco y nodo en lista de adjacencia
{
    int edge; int node;
    pairCC(int e, int n): edge(e), node(n) {}
    string print_pair()
    {
        stringstream ss;
        ss << edge << ";" << node << endl;
        return ss.str();
    }        // end print_solution
};

class Graph
{
    int V; // No. of vertices
    list<pairCC> *adj; // An array of adjacency lists ,keeps id of edge and id of node
    list<pairCC> *adjb; // adjacency list but backward
    
    // A recursive function to print DFS starting from v
	void DFSUtil(int v, vector<bool> visited);
public:
    // Constructor and Destructor
	Graph()
	{
	}
	Graph(int V)
    {
		this->setSize(V);
    }
	void setSize(int V)
	{
		this->V = V;
		adj = new list<pairCC>[V];
		adjb = new list<pairCC>[V];
	}
    ~Graph()
    {
        delete[] adj;
        delete[] adjb;
    }
    
    // Method to add an edge
    void addEdge(int v, int w, int ne, bool pred, vector<Edge>& Edges, vector<string> &finalp, bool super, int descarga, vector<Producto>& Prod); // agrega arco (v,w), que en el caso de tener dos etapas, se conecta a nodo ne, el cual es es predecesor si pred = true, antecesor si pred = false, false en caso de no tener ni uno ni el otro
    int checkEdge(string NO, string ND, vector<Edge>& Edges, vector<Node>& Nodes);  // retorna la posición del vector comparado con exito en Edges, -1 en caso que no encuentra
    void addAtributo(int edge_id, int ne, bool pred, vector<Edge>& Edges, vector<string> &finalp, bool super, int descarga, vector<Producto>& Prod); // agrega atributo a arco edge_id, que en el caso de tener dos etapas, se conecta a nodo ne, el cual es predecesor si pred = true, antecesor si pred = false, false en caso de no tener ni uno ni el otro
    void Find_nodes(vector<string> finalp, string* NOF, string* NDF, string* NDFF, bool* super, bool* nodeAtrib, int* nodeDescarga);
    void Actualiza_arcos_compuestos(vector<Edge>& Edges, vector<Node>& Nodes);
    int get_Node_id(string NF, vector<Node>& Nodes);
    
    // The main function that returns true if the graph is strongly
    // connected, otherwise false
    bool isSC();
    
    // Function that returns reverse (or transpose) of this graph
    Graph getTranspose();
    void InOut(int node_id, string type, vector<Edge>& Edges, vector<Node>& Nodes, vector<Producto>& Prod, int Indicador, double* outgoing, double* Incoming);
    void InOutXFiltrado(int node_id, string CODEProd, vector<Edge>& Edges, vector<Node>& Nodes, vector<Producto>& Prod, int Indicador, double* outgoing, double* Incoming, bool super);
    void InOutXTotalKilos(int node_id, vector<Edge>& Edges, vector<Node>& Nodes, vector<Producto>& Prod, int Indicador, double* outgoing, double* Incoming);
    //cuenta número de pallets de un cierto producto id_producto llegando a nodo fin de EdgeIN
    double Pallets_producto(string id_producto, vector<Producto>& Prod);
    void FillCamionesSucursal(int node_id, vector<Edge>& Edges, vector<Node>& Nodes, vector<int>& Marcados);   //llena camiones llegando a node_id y que requieren ser programados
    void AjustaItinerarios(vector<Edge>& Edges, vector<Node>& Nodes);  //itinerarios se ajustan al siguiente horario en 15 mins
    void FillCamionesPlanta(vector<Edge>& Edges, vector<Node>& Nodes);
    Camion FindCamionSaliente(int node_id, int camion_id, vector<Edge>& Edges);
    int FindEdgeFeeder(int node_id, int camion_id, int* pos, vector<Edge>& Edges);
    void FillCamionesFeeder(vector<Edge>& Edges, vector<Node>& Nodes);
    string PrintLabels1(vector<Edge>& Edges, vector<Node>& Nodes);
    string PrintLabels(vector<Edge>& Edges, vector<Node>& Nodes);
    vector<camion_identificador> Find_camion(int camion_id, vector<Edge>& Edges);
    int NodeID_planta1(int camion_id, vector<Edge>& Edges, vector<Node>& Nodes);
    vector<int> NodesID(int camion_id, vector<Edge>& Edges, vector<Node>& Nodes);
    string PrintRutaCamion(int camion_id, vector<Edge>& Edges, vector<Node>& Nodes);
    string GetInOutCapacidades(int node_id,vector<Edge>& Edges, vector<Node>& Nodes,vector<Producto>& Prod, vector<Familia>& FA);
    void Actualiza_tiempos_totales(vector<Edge>& Edges, vector<Node>& Nodes, int IdLastCamion);
    bool Find_camion_feeder_interplanta(vector<int>& IDcamiones, Camion CM, vector<Edge>& Edges, vector<Node>& Nodes);
    vector<vector<int>> IdentificaOrigen(string CodProducto, string ND, set<string>& Plantas, vector<Edge>& Edges, vector<Node>& Nodes, vector<Producto>& Prod, bool* primario);
    string PrintRutas(vector<Edge>& Edges, vector<Node>& Nodes, vector<Producto>& Prod, vector<string>& sectores);
    string PrintCuadratura(vector<Edge>& Edges, vector<Node>& Nodes, vector<Producto>& Prod);
    
};

// A recursive function to print DFS starting from v
void Graph::DFSUtil(int v, vector<bool> visited)
{
    // Mark the current node as visited and print it
    visited[v] = true;
    
    // Recur for all the vertices adjacent to this vertex
    list<pairCC>::iterator i;
    for (i = adj[v].begin(); i != adj[v].end(); ++i)
    {
        if (!visited[i->node])
        {
            cout << v << "-" << i->node << "nv ";
            DFSUtil(i->node, visited);
        }
//        cout << v << "-" << *i << "v ";
    }       // end for
}

// Function that returns reverse (or transpose) of this graph
Graph Graph::getTranspose()
{
    Graph g(V);
    for (int v = 0; v < V; v++)
    {
        // Recur for all the vertices adjacent to this vertex
        list<pairCC>::iterator i;
        for (i = adj[v].begin(); i != adj[v].end(); ++i)
        {
            g.adj[i->node].push_back(pairCC(0,v));
        }
    }
    return g;
}

void Graph::addEdge(int v, int w, int ne, bool pred, vector<Edge>& Edges, vector<string> &finalp, bool super, int descarga, vector<Producto>& Prod)
{
    int edge_id = (int) Edges.size();
    double cajasPorPallet = -1;
    if((finalp[0] == "x")||(finalp[0] == "qx")||(finalp[0] == "x2")||(finalp[0] == "qx2")||(finalp[0] == "xr" )||(finalp[0] == "qxr")||(finalp[0] == "xs" )||(finalp[0] == "qxs"))
    {
        int prod_id = get_Prod_id(finalp[1],Prod);
        cajasPorPallet = Prod[prod_id].get_CajasPallet();
    }
    
    Edges.push_back(Edge(edge_id,v,w));
    int p,s;
    if(pred)
    { p=-2; s=-1;}
    else
    { p=-1; s=-2;}
    
	if (((finalp[0] == "x")||(finalp[0] == "qx")) && (stod(finalp[5]) > UmbralMinimoAmount))
        Edges[edge_id].x.push_back(Edge_parts(finalp[0], finalp[1], stod(finalp[5]), stoi(finalp[2]), super,-1,-1,-1,-1,-1,-1,cajasPorPallet));
    
    else if (((finalp[0] == "x2")||(finalp[0] == "qx2")) && (stod(finalp[6]) > UmbralMinimoAmount))
    {
        Edges[edge_id].x2.push_back(Edge_parts(finalp[0], finalp[1], stod(finalp[6]), stoi(finalp[2]), super,-1,-1,p,s,ne,-1,cajasPorPallet));
    }
    
    else if (((finalp[0] == "xr" )||(finalp[0] == "qxr")) && (stod(finalp[6]) > UmbralMinimoAmount))
        Edges[edge_id].xr.push_back(Edge_parts(finalp[0], finalp[1], stod(finalp[6]), stoi(finalp[2]), super,-1,-1,p,s,ne,-1,cajasPorPallet));
    
    else if (((finalp[0] == "xs")||(finalp[0] == "qxs")) && (stod(finalp[7]) > UmbralMinimoAmount))
    {
        
        if((!pred)&&(descarga == 1))
            Edges[edge_id].xs.push_back(Edge_parts(finalp[0], finalp[1], stod(finalp[7]), stoi(finalp[2]),super,-1,-1,p,s,ne,1,cajasPorPallet));
        else if((!pred)&&(descarga == 2))
            Edges[edge_id].xs.push_back(Edge_parts(finalp[0], finalp[1], stod(finalp[7]), stoi(finalp[2]), super,-1,-1,p,s,ne,2,cajasPorPallet));
        else if((pred)&&(descarga == 2))
            Edges[edge_id].xs.push_back(Edge_parts(finalp[0], finalp[1], stod(finalp[7]), stoi(finalp[2]), super,-1,-1,p,s,ne,2,cajasPorPallet));
    }
       
    else if(((finalp[0] == "p")||(finalp[0] == "qp")) && (stod(finalp[4]) > UmbralMinimoAmount))
        Edges[edge_id].p.push_back(Edge_parts(finalp[0], finalp[3], stod(finalp[4]),-1, false,-1,-1,-1,-1,-1,-1,-1));
    
    else if(((finalp[0] == "p2")||(finalp[0] == "qp2")) && (stod(finalp[5]) > UmbralMinimoAmount))
        Edges[edge_id].p2.push_back(Edge_parts(finalp[0], finalp[4], stod(finalp[5]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if(((finalp[0] == "pr")||(finalp[0] == "qpr")) && (stod(finalp[5]) > UmbralMinimoAmount))
        Edges[edge_id].pr.push_back(Edge_parts(finalp[0], finalp[4], stod(finalp[5]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if(((finalp[0] == "z")||(finalp[0] == "qz")) && (stod(finalp[4]) > UmbralMinimoAmount))
        Edges[edge_id].z.push_back(Edge_parts(finalp[0], finalp[3], stod(finalp[4]),-1, false,-1,-1,-1,-1,-1,-1,-1));
    
    else if(((finalp[0] == "z2")||(finalp[0] == "qz2")) && (stod(finalp[5]) > UmbralMinimoAmount))
           Edges[edge_id].z2.push_back(Edge_parts(finalp[0], finalp[4], stod(finalp[5]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if(((finalp[0] == "zr")||(finalp[0] == "qzr")) && (stod(finalp[5]) > UmbralMinimoAmount))
    Edges[edge_id].zr.push_back(Edge_parts(finalp[0], finalp[4], stod(finalp[5]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if (((finalp[0] == "zs")||(finalp[0] == "qzs")) && (stod(finalp[6]) > UmbralMinimoAmount))
    {
        if((!pred)&&(descarga == 1))
            Edges[edge_id].zs.push_back(Edge_parts(finalp[0], finalp[5], stod(finalp[6]), -1, false,-1,-1,p,s,ne,1,-1));
        else if((!pred)&&(descarga == 2))
            Edges[edge_id].zs.push_back(Edge_parts(finalp[0], finalp[5], stod(finalp[6]), -1, false,-1,-1,p,s,ne,2,-1));
        else if((pred)&&(descarga == 2))
            Edges[edge_id].zs.push_back(Edge_parts(finalp[0], finalp[5], stod(finalp[6]), -1, false,-1,-1,p,s,ne,2,-1));
    }
    
    else if(((finalp[0] == "zc")||(finalp[0] == "qzc")) && (stod(finalp[3]) > UmbralMinimoAmount))
           Edges[edge_id].zc.push_back(Edge_parts(finalp[0],"-1", stod(finalp[3]),-1, false,-1,-1,-1,-1,-1,-1,-1));
       
    else if(((finalp[0] == "z2c")||(finalp[0] == "qz2c")) && (stod(finalp[4]) > UmbralMinimoAmount))
            Edges[edge_id].z2c.push_back(Edge_parts(finalp[0], "-1", stod(finalp[4]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if(((finalp[0] == "zrc")||(finalp[0] == "qzrc")) && (stod(finalp[4]) > UmbralMinimoAmount))
    Edges[edge_id].zrc.push_back(Edge_parts(finalp[0], "-1", stod(finalp[4]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if(finalp[0] == "y" && stod(finalp[4]) > UmbralMinimoAmount)
        Edges[edge_id].y.push_back(Edge_parts(finalp[0], finalp[3], stod(finalp[4]),-1, false,-1,-1,-1,-1,-1,-1,-1));
    
    else if(finalp[0] == "y2" && stod(finalp[5]) > UmbralMinimoAmount)
           Edges[edge_id].y2.push_back(Edge_parts(finalp[0], finalp[4], stod(finalp[5]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if(finalp[0] == "ys" && stod(finalp[5]) > UmbralMinimoAmount)
           Edges[edge_id].ys.push_back(Edge_parts(finalp[0], finalp[4], stod(finalp[5]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    adj[v].push_back(pairCC(edge_id,w)); // Add w to v’s list.
    adjb[w].push_back(pairCC(edge_id,v));   // Add v to w´s list backward
    
}

void Graph::addAtributo(int edge_id, int ne, bool pred, vector<Edge>& Edges, vector<string> &finalp, bool super, int descarga, vector<Producto>& Prod)
{
	int p,s;
    if(pred)
    { p=-2; s=-1;}
    else
    { p=-1; s=-2;}
    
    double cajasPorPallet = -1;
    if((finalp[0] == "x")||(finalp[0] == "qx")||(finalp[0] == "x2")||(finalp[0] == "qx2")||(finalp[0] == "xr" )||(finalp[0] == "qxr")||(finalp[0] == "xs" )||(finalp[0] == "qxs"))
    {
        int prod_id = get_Prod_id(finalp[1],Prod);
        cajasPorPallet = Prod[prod_id].get_CajasPallet();
    }
    
    if (((finalp[0] == "x")||(finalp[0] == "qx")) && (stod(finalp[5]) > UmbralMinimoAmount))
        Edges[edge_id].x.push_back(Edge_parts(finalp[0], finalp[1], stod(finalp[5]), stoi(finalp[2]), super,-1,-1,-1,-1,-1,-1,cajasPorPallet));
    
    else if (((finalp[0] == "x2")||(finalp[0] == "qx2")) && (stod(finalp[6]) > UmbralMinimoAmount))
        Edges[edge_id].x2.push_back(Edge_parts(finalp[0], finalp[1], stod(finalp[6]), stoi(finalp[2]), super,-1,-1,p,s,ne,-1,cajasPorPallet));
    
    else if (((finalp[0] == "xr" )||(finalp[0] == "qxr")) && (stod(finalp[6]) > UmbralMinimoAmount))
        Edges[edge_id].xr.push_back(Edge_parts(finalp[0], finalp[1], stod(finalp[6]), stoi(finalp[2]), super,-1,-1,p,s,ne,-1,cajasPorPallet));
    
    else if (((finalp[0] == "xs")||(finalp[0] == "qxs")) && (stod(finalp[7]) > UmbralMinimoAmount))
    {
        
        if((!pred)&&(descarga == 1))
            Edges[edge_id].xs.push_back(Edge_parts(finalp[0], finalp[1], stod(finalp[7]), stoi(finalp[2]),super,-1,-1,p,s,ne,1,cajasPorPallet));
        else if((!pred)&&(descarga == 2))
            Edges[edge_id].xs.push_back(Edge_parts(finalp[0], finalp[1], stod(finalp[7]), stoi(finalp[2]), super,-1,-1,p,s,ne,2,cajasPorPallet));
        else if((pred)&&(descarga == 2))
            Edges[edge_id].xs.push_back(Edge_parts(finalp[0], finalp[1], stod(finalp[7]), stoi(finalp[2]), super,-1,-1,p,s,ne,2,cajasPorPallet));
    }
    
    else if(((finalp[0] == "p")||(finalp[0] == "qp")) && (stod(finalp[4]) > UmbralMinimoAmount))
        Edges[edge_id].p.push_back(Edge_parts(finalp[0], finalp[3], stod(finalp[4]),-1, false,-1,-1,-1,-1,-1,-1,-1));
    
    else if(((finalp[0] == "p2")||(finalp[0] == "qp2")) && (stod(finalp[5]) > UmbralMinimoAmount))
        Edges[edge_id].p2.push_back(Edge_parts(finalp[0], finalp[4], stod(finalp[5]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if(((finalp[0] == "pr")||(finalp[0] == "qpr")) && (stod(finalp[5]) > UmbralMinimoAmount))
        Edges[edge_id].pr.push_back(Edge_parts(finalp[0], finalp[4], stod(finalp[5]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if(((finalp[0] == "z")||(finalp[0] == "qz")) && (stod(finalp[4]) > UmbralMinimoAmount))
        Edges[edge_id].z.push_back(Edge_parts(finalp[0], finalp[3], stod(finalp[4]),-1, false,-1,-1,-1,-1,-1,-1,-1));
    
    else if(((finalp[0] == "z2")||(finalp[0] == "qz2")) && (stod(finalp[5]) > UmbralMinimoAmount))
           Edges[edge_id].z2.push_back(Edge_parts(finalp[0], finalp[4], stod(finalp[5]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if(((finalp[0] == "zr")||(finalp[0] == "qzr")) && (stod(finalp[5]) > UmbralMinimoAmount))
    Edges[edge_id].zr.push_back(Edge_parts(finalp[0], finalp[4], stod(finalp[5]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if (((finalp[0] == "zs")||(finalp[0] == "qzs")) && (stod(finalp[6]) > UmbralMinimoAmount))
    {
        if((!pred)&&(descarga == 1))
            Edges[edge_id].zs.push_back(Edge_parts(finalp[0], finalp[5], stod(finalp[6]), -1, false,-1,-1,p,s,ne,1,-1));
        else if((!pred)&&(descarga == 2))
            Edges[edge_id].zs.push_back(Edge_parts(finalp[0], finalp[5], stod(finalp[6]), -1, false,-1,-1,p,s,ne,2,-1));
        else if((pred)&&(descarga == 2))
            Edges[edge_id].zs.push_back(Edge_parts(finalp[0], finalp[5], stod(finalp[6]), -1, false,-1,-1,p,s,ne,2,-1));
    }
    
    else if(((finalp[0] == "zc")||(finalp[0] == "qzc")) && (stod(finalp[3]) > UmbralMinimoAmount))
           Edges[edge_id].zc.push_back(Edge_parts(finalp[0],"-1", stod(finalp[3]),-1, false,-1,-1,-1,-1,-1,-1,-1));
       
    else if(((finalp[0] == "z2c")||(finalp[0] == "qz2c")) && (stod(finalp[4]) > UmbralMinimoAmount))
            Edges[edge_id].z2c.push_back(Edge_parts(finalp[0], "-1", stod(finalp[4]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if(((finalp[0] == "zrc")||(finalp[0] == "qzrc")) && (stod(finalp[4]) > UmbralMinimoAmount))
    Edges[edge_id].zrc.push_back(Edge_parts(finalp[0], "-1", stod(finalp[4]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if(finalp[0] == "y" && stod(finalp[4]) > UmbralMinimoAmount)
        Edges[edge_id].y.push_back(Edge_parts(finalp[0], finalp[3], stod(finalp[4]),-1, false,-1,-1,-1,-1,-1,-1,-1));
    
    else if(finalp[0] == "y2" && stod(finalp[5]) > UmbralMinimoAmount)
           Edges[edge_id].y2.push_back(Edge_parts(finalp[0], finalp[4], stod(finalp[5]),-1, false,-1,-1,p,s,ne,-1,-1));
    
    else if(finalp[0] == "ys" && stod(finalp[5]) > UmbralMinimoAmount)
    {
        Edges[edge_id].ys.push_back(Edge_parts(finalp[0], finalp[4], stod(finalp[5]),-1, false,-1,-1,p,s,ne,-1,-1));
    }
    
}       // end addAtributo

void Graph::Find_nodes(vector<string> finalp, string* NOF, string* NDF, string* NDFF, bool* super, bool* nodeAtrib, int* nodeDescarga)
{
    *super = false;
    *nodeAtrib = false;
    *nodeDescarga = -1;
    
    if((finalp[0] == "x")||(finalp[0] == "qx"))
    {
        *NOF = finalp[3];
        if(finalp[4].back() == 'S')
        {
            finalp[4].pop_back();
            *super = true;
        }
        *NDF = finalp[4];
        NDFF = NULL;
    }
    else if((finalp[0] == "x2")||(finalp[0] == "qx2"))
    {
        *NOF = finalp[3];
        *NDF = finalp[4];
        if(finalp[5].back() == 'S')
        {
            finalp[5].pop_back();
            *super = true;
        }
        *NDFF = finalp[5];
    }
    
    else if((finalp[0] == "xr")||(finalp[0] == "qxr"))
       {
           *NOF = finalp[3];
           *NDF = finalp[4];
           if(finalp[5].back() == 'S')
           {
               finalp[5].pop_back();
               *super = true;
           }
           *NDFF = finalp[5];
       }
    
    else if((finalp[0] == "xs")||(finalp[0] == "qxs"))
    {
        *NOF = finalp[4];
        *NDF = finalp[5];
        *NDFF = finalp[6];
        
        if(finalp[3].back() == 'S')
        {
            finalp[3].pop_back();
            *super = true;
        }
        // acá se indica donde se van a descargar estas cajas
        if(finalp[3] == finalp[5])
            *nodeDescarga = 1;
        else if(finalp[3] == finalp[6])
            *nodeDescarga = 2;
        
//      cout << "CheckDesc: " << *NOF << ":" << *NDF << ":" << *NDFF << ":" << finalp[3] << ":" << *nodeDescarga << endl;
        
    }
    
    else if((finalp[0] == "p") || (finalp[0] == "z") || (finalp[0] == "zc") || (finalp[0] == "y")||(finalp[0] == "qp") || (finalp[0] == "qz") || (finalp[0] == "qzc"))
    {
        *NOF = finalp[1];
        *NDF = finalp[2];
        NDFF = NULL;
    }
    
    else if((finalp[0] == "p2") || (finalp[0] == "z2") || (finalp[0] == "z2c") || (finalp[0] == "y2") || (finalp[0] == "pr") || (finalp[0] == "zr") || (finalp[0] == "zrc")||(finalp[0] == "qp2") || (finalp[0] == "qz2") || (finalp[0] == "qz2c") || (finalp[0] == "qpr") || (finalp[0] == "qzr") || (finalp[0] == "qzrc") || (finalp[0] == "ys"))
    {
        *NOF = finalp[1];
        *NDF = finalp[2];
        *NDFF = finalp[3];
    }
    
    else if((finalp[0] == "zs") || (finalp[0] == "qzs"))
    {
        *NOF = finalp[2];
        *NDF = finalp[3];
        *NDFF = finalp[4];
        // acá se indica donde se van a descargar esots pallets
        if(finalp[1] == finalp[3])
            *nodeDescarga = 1;
        else if(finalp[1] == finalp[4])
            *nodeDescarga = 2;
    }
    
    else if((finalp[0] == "o1") || (finalp[0] == "o2")||(finalp[0] == "qo1") || (finalp[0] == "qo2"))
    {
       *NOF = finalp[3];
       NDF = NULL;
       NDFF = NULL;
       *nodeAtrib = true;
    }
    else if((finalp[0] == "u")||(finalp[0] == "qu"))
    {
        if(finalp[3].back() == 'S')
        {
            finalp[3].pop_back();
            *super = true;
        }
       *NOF = finalp[3];
       NDF = NULL;
       NDFF = NULL;
       *nodeAtrib = true;
    }
}       // end find_nodes

// fn que actualiza la información de edgepart que es complemento de cada arco
void Graph::Actualiza_arcos_compuestos(vector<Edge>& Edges, vector<Node>& Nodes)
{
    for(unsigned int i=0; i<Edges.size(); i++)
    {
       // Tratamiento para x2 ************************************
        if(Edges[i].x2.size() > 0)
        {
            for(unsigned int k=0; k<Edges[i].x2.size(); k++)
            {
                if((Edges[i].x2[k].id_predec == -2) && (Edges[i].x2[k].id_sucesor == -2))
                {
					fprintf(stderr, "Algo malo en actualia arcos compuestos, predecesor y sucesor ambos activados.\n");
					exit(1);
                }
                if(Edges[i].x2[k].id_predec == -2)      //Tengo que revisar la secuencia de predecesores
                {
                    int nd = Edges[i].v();
                    int no = Edges[i].x2[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adjb[nd].begin(); m != adjb[nd].end(); ++m)
                    {
                        if(m->node == no)
                        {
                            for (unsigned int j=0; j<Edges[m->edge].x2.size(); j++)
                            {
                                if(CompareEdgeParts(Edges[i].x2[k], Edges[m->edge].x2[j]))
                                {
                                    Edges[i].x2[k].id_predec = m->edge;
                                    Edges[i].x2[k].id_ep = j;
                                }
                            }
                            
                        }
                    }
                
            }
                else if(Edges[i].x2[k].id_sucesor == -2)    //Tengo que revisar la secuencia de sucesores
                {
                    int no = Edges[i].w();
                    int nd = Edges[i].x2[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adj[no].begin(); m != adj[no].end(); ++m)
                    {
                        if(m->node == nd)
                        {
                            for (unsigned int j=0; j<Edges[m->edge].x2.size(); j++)
                            {
                                if(CompareEdgeParts(Edges[i].x2[k], Edges[m->edge].x2[j]))
                                {
                                    Edges[i].x2[k].id_sucesor = m->edge;
                                    Edges[i].x2[k].id_ep = j;
                                }
                            }
                            
                        }
                    }
                }
            
            }
        }           // end tratamiento de x2 *************************+
        
        // Tratamiento para xr ************************************
               if(Edges[i].xr.size() > 0)
               {
                   for(unsigned int k=0; k<Edges[i].xr.size(); k++)
                   {
                       if((Edges[i].xr[k].id_predec == -2) && (Edges[i].xr[k].id_sucesor == -2))
                       {
						   fprintf(stderr, "Algo malo en actualia arcos compuestos, predecesor y sucesor ambos activados.\n");
						   exit(1);
                       }
                       if(Edges[i].xr[k].id_predec == -2)      //Tengo que revisar la secuencia de predecesores
                       {
                           int nd = Edges[i].v();
                           int no = Edges[i].xr[k].node_id;
                           list<pairCC>::iterator m;
                           for (m = adjb[nd].begin(); m != adjb[nd].end(); ++m)
                           {
                               if(m->node == no)
                               {
                                   for (unsigned int j=0; j<Edges[m->edge].xr.size(); j++)
                                   {
                                       if(CompareEdgeParts(Edges[i].xr[k], Edges[m->edge].xr[j]))
                                       {
                                           Edges[i].xr[k].id_predec = m->edge;
                                           Edges[i].xr[k].id_ep = j;
                                       }
                                   }
                                   
                               }
                           }
                       
                   }
                       else if(Edges[i].xr[k].id_sucesor == -2)    //Tengo que revisar la secuencia de sucesores
                       {
                           int no = Edges[i].w();
                           int nd = Edges[i].xr[k].node_id;
                           list<pairCC>::iterator m;
                           for (m = adj[no].begin(); m != adj[no].end(); ++m)
                           {
                               if(m->node == nd)
                               {
                                   for (unsigned int j=0; j<Edges[m->edge].xr.size(); j++)
                                   {
                                       if(CompareEdgeParts(Edges[i].xr[k], Edges[m->edge].xr[j]))
                                       {
                                           Edges[i].xr[k].id_sucesor = m->edge;
                                           Edges[i].xr[k].id_ep = j;
                                       }
                                   }
                                   
                               }
                           }
                       }
                   
                   }
               }           // end tratamiento de xr *************************+
        
        
        // Tratamiento para xs ************************************
               if(Edges[i].xs.size() > 0)
               {
                   for(unsigned int k=0; k<Edges[i].xs.size(); k++)
                   {
                       if((Edges[i].xs[k].id_predec == -2) && (Edges[i].xs[k].id_sucesor == -2))
                       {
                           fprintf(stderr, "Algo malo en actualia arcos compuestos, predecesor y sucesor ambos activados.\n");
                           exit(1);
                       }
                       if(Edges[i].xs[k].id_predec == -2)      //Tengo que revisar la secuencia de predecesores
                       {
                           int nd = Edges[i].v();
                           int no = Edges[i].xs[k].node_id;
                           list<pairCC>::iterator m;
                           for (m = adjb[nd].begin(); m != adjb[nd].end(); ++m)
                           {
                               if(m->node == no)
                               {
                                   for (unsigned int j=0; j<Edges[m->edge].xs.size(); j++)
                                   {
                                       if(CompareEdgeParts(Edges[i].xs[k], Edges[m->edge].xs[j]))
                                       {
                                           Edges[i].xs[k].id_predec = m->edge;
                                           Edges[i].xs[k].id_ep = j;
                                       }
                                   }
                                   
                               }
                           }
                       
                   }
                       else if(Edges[i].xs[k].id_sucesor == -2)    //Tengo que revisar la secuencia de sucesores
                       {
                           int no = Edges[i].w();
                           int nd = Edges[i].xs[k].node_id;
                           list<pairCC>::iterator m;
                           for (m = adj[no].begin(); m != adj[no].end(); ++m)
                           {
                               if(m->node == nd)
                               {
                                   if(Edges[i].xs[k].descarga == 2)
                                   {
                                       for (unsigned int j=0; j<Edges[m->edge].xs.size(); j++)
                                       {
                                           if(CompareEdgeParts(Edges[i].xs[k], Edges[m->edge].xs[j]))
                                           {
                                               Edges[i].xs[k].id_sucesor = m->edge;
                                               Edges[i].xs[k].id_ep = j;
                                           }
                                       }        // end for
                                   }            // end if chequeando si tiene sucesor
                                   else if(Edges[i].xs[k].descarga == 1)
                                       Edges[i].xs[k].id_sucesor = m->edge;
                               }
                           }
                       }
                   
                   }
               }           // end tratamiento de xs *************************+
        
        
        // Tratamiento para p2 ************************************
        if(Edges[i].p2.size() > 0)
        {
            for(unsigned int k=0; k<Edges[i].p2.size(); k++)
            {
                if((Edges[i].p2[k].id_predec == -2) && (Edges[i].p2[k].id_sucesor == -2))
                {
					fprintf(stderr, "Algo malo en actualia arcos compuestos, predecesor y sucesor ambos activados.\n");
					exit(1);
                }
                if(Edges[i].p2[k].id_predec == -2)      //Tengo que revisar la secuencia de predecesores
                {
                    int nd = Edges[i].v();
                    int no = Edges[i].p2[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adjb[nd].begin(); m != adjb[nd].end(); ++m)
                    {
                        if(m->node == no)
                        {
                            for (unsigned int j=0; j<Edges[m->edge].p2.size(); j++)
                            {
                                if(CompareEdgeParts(Edges[i].p2[k], Edges[m->edge].p2[j]))
                                {
                                    Edges[i].p2[k].id_predec = m->edge;
                                    Edges[i].p2[k].id_ep = j;
                                }
                            }
                                   
                        }
                    }
                       
                }
                else if(Edges[i].p2[k].id_sucesor == -2)    //Tengo que revisar la secuencia de sucesores
                {
                    int no = Edges[i].w();
                    int nd = Edges[i].p2[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adj[no].begin(); m != adj[no].end(); ++m)
                    {
                        if(m->node == nd)
                        {
                            for (unsigned int j=0; j<Edges[m->edge].p2.size(); j++)
                            {
                                if(CompareEdgeParts(Edges[i].p2[k], Edges[m->edge].p2[j]))
                                {
                                    Edges[i].p2[k].id_sucesor = m->edge;
                                    Edges[i].p2[k].id_ep = j;
                                }
                            }
                                   
                        }
                    }
                }
                   
            }
        }           // end tratamiento de p2 *************************+
        
        // Tratamiento para pr ************************************
               if(Edges[i].pr.size() > 0)
               {
                   for(unsigned int k=0; k<Edges[i].pr.size(); k++)
                   {
                       if((Edges[i].pr[k].id_predec == -2) && (Edges[i].pr[k].id_sucesor == -2))
                       {
						   fprintf(stderr, "Algo malo en actualiza arcos compuestos, predecesor y sucesor ambos activados.\n");
						   exit(1);
                       }
                       if(Edges[i].pr[k].id_predec == -2)      //Tengo que revisar la secuencia de predecesores
                       {
                           int nd = Edges[i].v();
                           int no = Edges[i].pr[k].node_id;
                           list<pairCC>::iterator m;
                           for (m = adjb[nd].begin(); m != adjb[nd].end(); ++m)
                           {
                               if(m->node == no)
                               {
                                   for (unsigned int j=0; j<Edges[m->edge].pr.size(); j++)
                                   {
                                       if(CompareEdgeParts(Edges[i].pr[k], Edges[m->edge].pr[j]))
                                       {
                                           Edges[i].pr[k].id_predec = m->edge;
                                           Edges[i].pr[k].id_ep = j;
                                       }
                                   }
                                          
                               }
                           }
                              
                       }
                       else if(Edges[i].pr[k].id_sucesor == -2)    //Tengo que revisar la secuencia de sucesores
                       {
                           int no = Edges[i].w();
                           int nd = Edges[i].pr[k].node_id;
                           list<pairCC>::iterator m;
                           for (m = adj[no].begin(); m != adj[no].end(); ++m)
                           {
                               if(m->node == nd)
                               {
                                   for (unsigned int j=0; j<Edges[m->edge].pr.size(); j++)
                                   {
                                       if(CompareEdgeParts(Edges[i].pr[k], Edges[m->edge].pr[j]))
                                       {
                                           Edges[i].pr[k].id_sucesor = m->edge;
                                           Edges[i].pr[k].id_ep = j;
                                       }
                                   }
                                          
                               }
                           }
                       }
                          
                   }
               }           // end tratamiento de pr *************************+
               
        
        // Repetir procedmiento para z2
        if(Edges[i].z2.size() > 0)
        {
            for(unsigned int k=0; k<Edges[i].z2.size(); k++)
            {
                if((Edges[i].z2[k].id_predec == -2) && (Edges[i].z2[k].id_sucesor == -2))
                {
					fprintf(stderr, "Algo malo en actualiza arcos compuestos, predecesor y sucesor ambos activados.\n");
					exit(1);
                }
                if(Edges[i].z2[k].id_predec == -2)      //Tengo que revisar la secuencia de predecesores
                {
                    int nd = Edges[i].v();
                    int no = Edges[i].z2[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adjb[nd].begin(); m != adjb[nd].end(); ++m)
                    {
                        if(m->node == no)
                        {
                            for (unsigned int j=0; j<Edges[m->edge].z2.size(); j++)
                            {
                                if(CompareEdgeParts(Edges[i].z2[k], Edges[m->edge].z2[j]))
                                {
                                    Edges[i].z2[k].id_predec = m->edge;
                                    Edges[i].z2[k].id_ep = j;
                                }
                            }
                                   
                        }
                    }
                       
                }
                else if(Edges[i].z2[k].id_sucesor == -2)    //Tengo que revisar la secuencia de sucesores
                {
                    int no = Edges[i].w();
                    int nd = Edges[i].z2[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adj[no].begin(); m != adj[no].end(); ++m)
                    {
                        if(m->node == nd)
                        {
                            for (unsigned int j=0; j<Edges[m->edge].z2.size(); j++)
                            {
                                if(CompareEdgeParts(Edges[i].z2[k], Edges[m->edge].z2[j]))
                                {
                                    Edges[i].z2[k].id_sucesor = m->edge;
                                    Edges[i].z2[k].id_ep = j;
                                }
                            }
                                   
                        }
                    }
                }
                   
            }
        }           // end tratamiento de z2 *************************+
        
        // Repetir procedmiento para zs
        if(Edges[i].zs.size() > 0)
        {
            for(unsigned int k=0; k<Edges[i].zs.size(); k++)
            {
                if((Edges[i].zs[k].id_predec == -2) && (Edges[i].zs[k].id_sucesor == -2))
                {
                    fprintf(stderr, "Algo malo en actualiza arcos compuestos, predecesor y sucesor ambos activados.\n");
                    exit(1);
                }
                if(Edges[i].zs[k].id_predec == -2)      //Tengo que revisar la secuencia de predecesores
                {
                    int nd = Edges[i].v();
                    int no = Edges[i].zs[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adjb[nd].begin(); m != adjb[nd].end(); ++m)
                    {
                        if(m->node == no)
                        {
                            for (unsigned int j=0; j<Edges[m->edge].zs.size(); j++)
                            {
                                if(CompareEdgeParts(Edges[i].zs[k], Edges[m->edge].zs[j]))
                                {
                                    Edges[i].zs[k].id_predec = m->edge;
                                    Edges[i].zs[k].id_ep = j;
                                }
                            }
                                   
                        }
                    }
                       
                }
                else if(Edges[i].zs[k].id_sucesor == -2)    //Tengo que revisar la secuencia de sucesores
                {
                    int no = Edges[i].w();
                    int nd = Edges[i].zs[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adj[no].begin(); m != adj[no].end(); ++m)
                    {
                        if(m->node == nd)
                        {
                            if(Edges[i].zs[k].descarga == 2)
                            {
                                for (unsigned int j=0; j<Edges[m->edge].zs.size(); j++)
                                {
                                    if(CompareEdgeParts(Edges[i].zs[k], Edges[m->edge].zs[j]))
                                    {
                                        Edges[i].zs[k].id_sucesor = m->edge;
                                        Edges[i].zs[k].id_ep = j;
                                    }
                                }       // end for
                            }
                            else if(Edges[i].zs[k].descarga == 1)
                                Edges[i].zs[k].id_sucesor = m->edge;
            
                        }
                    }
                }
                   
            }
        }           // end tratamiento de zs *************************+
        
        
        // Repetir procedmiento para zr
               if(Edges[i].zr.size() > 0)
               {
                   for(unsigned int k=0; k<Edges[i].zr.size(); k++)
                   {
                       if((Edges[i].zr[k].id_predec == -2) && (Edges[i].zr[k].id_sucesor == -2))
                       {
						   fprintf(stderr, "Algo malo en actualiza arcos compuestos, predecesor y sucesor ambos activados.\n");
						   exit(1);
                       }
                       if(Edges[i].zr[k].id_predec == -2)      //Tengo que revisar la secuencia de predecesores
                       {
                           int nd = Edges[i].v();
                           int no = Edges[i].zr[k].node_id;
                           list<pairCC>::iterator m;
                           for (m = adjb[nd].begin(); m != adjb[nd].end(); ++m)
                           {
                               if(m->node == no)
                               {
                                   for (unsigned int j=0; j<Edges[m->edge].zr.size(); j++)
                                   {
                                       if(CompareEdgeParts(Edges[i].zr[k], Edges[m->edge].zr[j]))
                                       {
                                           Edges[i].zr[k].id_predec = m->edge;
                                           Edges[i].zr[k].id_ep = j;
                                       }
                                   }
                                          
                               }
                           }
                              
                       }
                       else if(Edges[i].zr[k].id_sucesor == -2)    //Tengo que revisar la secuencia de sucesores
                       {
                           int no = Edges[i].w();
                           int nd = Edges[i].zr[k].node_id;
                           list<pairCC>::iterator m;
                           for (m = adj[no].begin(); m != adj[no].end(); ++m)
                           {
                               if(m->node == nd)
                               {
                                   for (unsigned int j=0; j<Edges[m->edge].zr.size(); j++)
                                   {
                                       if(CompareEdgeParts(Edges[i].zr[k], Edges[m->edge].zr[j]))
                                       {
                                           Edges[i].zr[k].id_sucesor = m->edge;
                                           Edges[i].zr[k].id_ep = j;
                                       }
                                   }
                                          
                               }
                           }
                       }
                          
                   }
               }           // end tratamiento de zr *************************+
               
        
        // Repetir procedmiento para z2c
        if(Edges[i].z2c.size() > 0)
        {
            for(unsigned int k=0; k<Edges[i].z2c.size(); k++)
            {
                if((Edges[i].z2c[k].id_predec == -2) && (Edges[i].z2c[k].id_sucesor == -2))
                {
					fprintf(stderr, "Algo malo en actualiza arcos compuestos, predecesor y sucesor ambos activados.\n");
					exit(1);
                }
                if(Edges[i].z2c[k].id_predec == -2)      //Tengo que revisar la secuencia de predecesores
                {
                    int nd = Edges[i].v();
                    int no = Edges[i].z2c[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adjb[nd].begin(); m != adjb[nd].end(); ++m)
                    {
                        if(m->node == no)
                        {
                            for (unsigned int j=0; j<Edges[m->edge].z2c.size(); j++)
                            {
                                if(CompareEdgeParts(Edges[i].z2c[k], Edges[m->edge].z2c[j]))
                                {
                                    Edges[i].z2c[k].id_predec = m->edge;
                                    Edges[i].z2c[k].id_ep = j;
                                }
                            }
                                   
                        }
                    }
                       
                }
                else if(Edges[i].z2c[k].id_sucesor == -2)    //Tengo que revisar la secuencia de sucesores
                {
                    int no = Edges[i].w();
                    int nd = Edges[i].z2c[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adj[no].begin(); m != adj[no].end(); ++m)
                    {
                        if(m->node == nd)
                        {
                            for (unsigned int j=0; j<Edges[m->edge].z2c.size(); j++)
                            {
                                if(CompareEdgeParts(Edges[i].z2c[k], Edges[m->edge].z2c[j]))
                                {
                                    Edges[i].z2c[k].id_sucesor = m->edge;
                                    Edges[i].z2c[k].id_ep = j;
                                }
                            }
                                   
                        }
                    }
                }
                   
            }
        }           // end tratamiento de z2c *************************+
        
        // Repetir procedmiento para zrc
               if(Edges[i].zrc.size() > 0)
               {
                   for(unsigned int k=0; k<Edges[i].zrc.size(); k++)
                   {
                       if((Edges[i].zrc[k].id_predec == -2) && (Edges[i].zrc[k].id_sucesor == -2))
                       {
						   fprintf(stderr, "Algo malo en actualiza arcos compuestos, predecesor y sucesor ambos activados.\n");
						   exit(1);
                       }
                       if(Edges[i].zrc[k].id_predec == -2)      //Tengo que revisar la secuencia de predecesores
                       {
                           int nd = Edges[i].v();
                           int no = Edges[i].zrc[k].node_id;
                           list<pairCC>::iterator m;
                           for (m = adjb[nd].begin(); m != adjb[nd].end(); ++m)
                           {
                               if(m->node == no)
                               {
                                   for (unsigned int j=0; j<Edges[m->edge].zrc.size(); j++)
                                   {
                                       if(CompareEdgeParts(Edges[i].zrc[k], Edges[m->edge].zrc[j]))
                                       {
                                           Edges[i].zrc[k].id_predec = m->edge;
                                           Edges[i].zrc[k].id_ep = j;
                                       }
                                   }
                                          
                               }
                           }
                              
                       }
                       else if(Edges[i].zrc[k].id_sucesor == -2)    //Tengo que revisar la secuencia de sucesores
                       {
                           int no = Edges[i].w();
                           int nd = Edges[i].zrc[k].node_id;
                           list<pairCC>::iterator m;
                           for (m = adj[no].begin(); m != adj[no].end(); ++m)
                           {
                               if(m->node == nd)
                               {
                                   for (unsigned int j=0; j<Edges[m->edge].zrc.size(); j++)
                                   {
                                       if(CompareEdgeParts(Edges[i].zrc[k], Edges[m->edge].zrc[j]))
                                       {
                                           Edges[i].zrc[k].id_sucesor = m->edge;
                                           Edges[i].zrc[k].id_ep = j;
                                       }
                                   }
                                          
                               }
                           }
                       }
                          
                   }
               }           // end tratamiento de zrc *************************+
        
        // Repetir procedmiento para y2
        if(Edges[i].y2.size() > 0)
        {
            for(unsigned int k=0; k<Edges[i].y2.size(); k++)
            {
                if((Edges[i].y2[k].id_predec == -2) && (Edges[i].y2[k].id_sucesor == -2))
                {
					fprintf(stderr, "Algo malo en actualiza arcos compuestos, predecesor y sucesor ambos activados.\n");
					exit(1);
                }
                if(Edges[i].y2[k].id_predec == -2)      //Tengo que revisar la secuencia de predecesores
                {
                    int nd = Edges[i].v();
                    int no = Edges[i].y2[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adjb[nd].begin(); m != adjb[nd].end(); ++m)
                    {
                        if(m->node == no)
                        {
                            for (unsigned int j=0; j<Edges[m->edge].y2.size(); j++)
                            {
                                if(CompareEdgeParts(Edges[i].y2[k], Edges[m->edge].y2[j]))
                                {
                                    Edges[i].y2[k].id_predec = m->edge;
                                    Edges[i].y2[k].id_ep = j;
                                }
                            }
                                   
                        }
                    }
                       
                }
                else if(Edges[i].y2[k].id_sucesor == -2)    //Tengo que revisar la secuencia de sucesores
                {
                    int no = Edges[i].w();
                    int nd = Edges[i].y2[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adj[no].begin(); m != adj[no].end(); ++m)
                    {
                        if(m->node == nd)
                        {
                            for (unsigned int j=0; j<Edges[m->edge].y2.size(); j++)
                            {
                                if(CompareEdgeParts(Edges[i].y2[k], Edges[m->edge].y2[j]))
                                {
                                    Edges[i].y2[k].id_sucesor = m->edge;
                                    Edges[i].y2[k].id_ep = j;
                                }
                            }
                                   
                        }
                    }
                }
                   
            }
        }           // end tratamiento de y2 *************************+
        
        // Repetir procedmiento para ys
        if(Edges[i].ys.size() > 0)
        {
            for(unsigned int k=0; k<Edges[i].ys.size(); k++)
            {
                if((Edges[i].ys[k].id_predec == -2) && (Edges[i].ys[k].id_sucesor == -2))
                {
                    fprintf(stderr, "Algo malo en actualiza arcos compuestos, predecesor y sucesor ambos activados.\n");
                    exit(1);
                }
                if(Edges[i].ys[k].id_predec == -2)      //Tengo que revisar la secuencia de predecesores
                {
                    int nd = Edges[i].v();
                    int no = Edges[i].ys[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adjb[nd].begin(); m != adjb[nd].end(); ++m)
                    {
                        if(m->node == no)
                        {
                            for (unsigned int j=0; j<Edges[m->edge].ys.size(); j++)
                            {
                                if(CompareEdgeParts(Edges[i].ys[k], Edges[m->edge].ys[j]))
                                {
                                    Edges[i].ys[k].id_predec = m->edge;
                                    Edges[i].ys[k].id_ep = j;
                                }
                            }
                                   
                        }
                    }
                       
                }
                else if(Edges[i].ys[k].id_sucesor == -2)    //Tengo que revisar la secuencia de sucesores
                {
                    int no = Edges[i].w();
                    int nd = Edges[i].ys[k].node_id;
                    list<pairCC>::iterator m;
                    for (m = adj[no].begin(); m != adj[no].end(); ++m)
                    {
                        if(m->node == nd)
                        {
                            for (unsigned int j=0; j<Edges[m->edge].ys.size(); j++)
                            {
                                if(CompareEdgeParts(Edges[i].ys[k], Edges[m->edge].ys[j]))
                                {
                                    Edges[i].ys[k].id_sucesor = m->edge;
                                    Edges[i].ys[k].id_ep = j;
                                }
                            }
                                   
                        }
                    }
                }
                   
            }
        }           // end tratamiento de ys *************************+
        
    }   // end for sobre arcos
    
}   // finaliza actualiza arcos compuestos


int Graph::get_Node_id(string NF, vector<Node>& Nodes)
{
    for(unsigned int k = 0; k < Nodes.size(); k++)
    {
       if(NF.compare(Nodes[k].get_cod()) == 0)
           return(k);
    }
    return(-1);     //node ID no fue encontrado...acá hay un error
}

int Graph::checkEdge(string NO, string ND, vector<Edge>& Edges, vector<Node>& Nodes)
{
    int no = get_Node_id(NO,Nodes);
    if(no == -1)        // nodo no está en la lista de nodos usados
        return(-1);
    
    list<pairCC>::iterator i;
    for (i = adj[no].begin(); i != adj[no].end(); ++i)
    {
        if(Nodes[i->node].get_cod() == ND)
            return(i->edge);
    }
    return(-1);
}

// The main function that returns true if graph is strongly connected
bool Graph::isSC()
{
    // St1p 1: Mark all the vertices as not visited (For first DFS)
    vector<bool> visited(V);
    for (int i = 0; i < V; i++)
        visited[i] = false;
    
    // Step 2: Do DFS traversal starting from first vertex.
    DFSUtil(0, visited);
    
    cout << " fin " << endl;
    
    // If DFS traversal doesn’t visit all vertices, then return false.
    for (int i = 0; i < V; i++)
        if (visited[i] == false)
            return false;
    
    // Step 3: Create a reversed graph
    Graph gr = getTranspose();
    
    // Step 4: Mark all the vertices as not visited (For second DFS)
    for (int i = 0; i < V; i++)
        visited[i] = false;
    
    // Step 5: Do DFS for reversed graph starting from first vertex.
    // Staring Vertex must be same starting point of first DFS
    gr.DFSUtil(0, visited);
    
    // If all vertices are not visited in second DFS, then
    // return false
    for (int i = 0; i < V; i++)
        if (visited[i] == false)
            return false;
    
    return true;
}

void Graph::InOut(int node_id, string type, vector<Edge>& Edges, vector<Node>& Nodes, vector<Producto>& Prod, int Indicador, double* outgoing, double* Incoming)
{
    double InF = 0;
    double OutF = 0;
    
    list<pairCC>::iterator i;
    for (i = adj[node_id].begin(); i != adj[node_id].end(); ++i)
    {
        if(type == "x")
            OutF += Edges[i->edge].sumCajasX_camionesTodo(Prod,"-1", Indicador);
        
        else if(type == "p")
            OutF += Edges[i->edge].sumUsoX_camiones(Prod,"-1", Indicador);
        
        else if(type == "z")
            OutF += Edges[i->edge].sumPallets_camiones(Prod, Indicador);
        
        else if(type == "y")        // REVISAR !!!
        {
            if(Indicador == 1)
                OutF += Edges[i->edge].fleet_size();
            else if(Indicador == 2)
                OutF += Edges[i->edge].CamionesD.size();
            else if(Indicador == 3)
            {
                if(Edges[i->edge].Camiones2.size()>0)
                {
                    for(unsigned int k=0; k<Edges[i->edge].Camiones2.size(); k++)
                        OutF += Edges[i->edge].Camiones2[k].size();
                }
                if(Edges[i->edge].CamionesS.size()>0)
                {
                    for(unsigned int k=0; k<Edges[i->edge].CamionesS.size(); k++)
                        OutF += Edges[i->edge].CamionesS[k].size();
                }
            }
        }       //en type y
    }       // end for
    
    list<pairCC>::iterator j;
    for (j = adjb[node_id].begin(); j != adjb[node_id].end(); ++j)
    {
        if(type == "x")
            InF += Edges[j->edge].sumCajasX_camionesTodo(Prod,"-1", Indicador);
        
        else if(type == "p")
            InF += Edges[j->edge].sumUsoX_camiones(Prod,"-1", Indicador);
        
        else if(type == "z")
            InF += Edges[j->edge].sumPallets_camiones(Prod, Indicador);
        
        else if(type == "y")
        {
            if(Indicador == 1)
                InF += Edges[j->edge].fleet_size();
            else if(Indicador == 2)
                InF += Edges[j->edge].CamionesD.size();
            else if(Indicador == 3)
            {
                if(Edges[j->edge].Camiones2.size()>0)
                {
                    for(unsigned int k=0; k<Edges[j->edge].Camiones2.size(); k++)
                        InF += Edges[j->edge].Camiones2[k].size();
                }
                if(Edges[j->edge].CamionesS.size()>0)
                {
                    for(unsigned int k=0; k<Edges[j->edge].CamionesS.size(); k++)
                        InF += Edges[j->edge].CamionesS[k].size();
                }
            }
        }
    }       // end for
    
    *outgoing = OutF;
    *Incoming = InF;
    
}   // end INOut

// Indicador vale 1 si proceso Camiones2, CamionesS y CamionesD, vale 2 si proceso sólo CamionesD y vale 3 si proceso solo Camiones2 y CamionesS
void Graph::InOutXFiltrado(int node_id, string CODEProd, vector<Edge>& Edges, vector<Node>& Nodes, vector<Producto>& Prod, int Indicador, double* outgoing, double* Incoming, bool super)
{
    double InF = 0;
    double OutF = 0;
    
    list<pairCC>::iterator i;
    for (i = adj[node_id].begin(); i != adj[node_id].end(); ++i)
        OutF += Edges[i->edge].sumCajasX_camiones(Prod, CODEProd, Indicador, super);
    
    list<pairCC>::iterator j;
    for (j = adjb[node_id].begin(); j != adjb[node_id].end(); ++j)
        InF += Edges[j->edge].sumCajasX_camiones(Prod, CODEProd, Indicador, super);
    
    *outgoing = OutF;
    *Incoming = InF;
    
}   // end INOut

// Indicador vale 1 si proceso Camiones2, CamionesS y CamionesD, vale 2 si proceso sólo CamionesD y vale 3 si proceso solo Camiones2 y CamionesS
void Graph::InOutXTotalKilos(int node_id, vector<Edge>& Edges, vector<Node>& Nodes, vector<Producto>& Prod, int Indicador, double* outgoing, double* Incoming)
{
    double InF = 0;
    double OutF = 0;
    
    list<pairCC>::iterator i;
    for (i = adj[node_id].begin(); i != adj[node_id].end(); ++i)
        OutF += Edges[i->edge].sumUsoX_camiones(Prod, "-1", Indicador);
    
    list<pairCC>::iterator j;
    for (j = adjb[node_id].begin(); j != adjb[node_id].end(); ++j)
        InF += Edges[j->edge].sumUsoX_camiones(Prod, "-1", Indicador);
    
    *outgoing = OutF;
    *Incoming = InF;
    
}   // end InOutXTotalKilos

// SE PRENDE SIEMPRE CONSIDERANDO TODOS LOS CAMIONES QUE LLEGAN A ESTE NODO SUCURSAL Y QUE NO HAN SIDO ASIGNADOS. En caso de viajes P-S-S, esta fn asigna última sucursal solamente
void Graph::FillCamionesSucursal(int node_id, vector<Edge>& Edges, vector<Node>& Nodes, vector<int>& Marcados)
{
    
    if(Nodes[node_id].get_type() == 2)  //fn sólo revisa sucursales
    {
        double theta = 24 - DeltaTime;
        list<pairCC>::iterator j;
        for (j = adjb[node_id].begin(); j != adjb[node_id].end(); ++j)
        {
            for(unsigned int k=0; k< Edges[j->edge].CamionesD.size(); k++)
            {
                vector<int> IDC;
                bool Feeder = Find_camion_feeder_interplanta(IDC, Edges[j->edge].CamionesD[k], Edges, Nodes);
                int NDias = ceil(Edges[j->edge].CamionesD[k].TTotal/24);
                
                int ND0 = NDias;
                
                for(unsigned int i=0; i<Marcados.size(); i++)
                {
                    if(Marcados[i] == Edges[j->edge].CamionesD[k].get_idc())
                        NDias--;
                }
                
                if((Edges[j->edge].CamionesD[k].TTotal < theta) || (NDias == 0))     //todo se programa el mismo día cero
                {
                    Nodes[node_id].Schedule0.push_back(trioProgramacion(Edges[j->edge].CamionesD[k].get_idc(),Edges[j->edge].CamionesD[k].TTotal, Feeder,Nodes[Edges[j->edge].v()].get_cod()));
                }
                else if(NDias == 1)             //un día de atraso
                {
                    Nodes[node_id].Schedule1.push_back(trioProgramacion(Edges[j->edge].CamionesD[k].get_idc(),Edges[j->edge].CamionesD[k].TTotal, Feeder, Nodes[Edges[j->edge].v()].get_cod()));
                }
                else if(NDias == 2)
                {
                    Nodes[node_id].Schedule2.push_back(trioProgramacion(Edges[j->edge].CamionesD[k].get_idc(),Edges[j->edge].CamionesD[k].TTotal,Feeder, Nodes[Edges[j->edge].v()].get_cod()));
                }
                else if(NDias >= 3)
                {
                    Nodes[node_id].Schedule3.push_back(trioProgramacion(Edges[j->edge].CamionesD[k].get_idc(),Edges[j->edge].CamionesD[k].TTotal,Feeder, Nodes[Edges[j->edge].v()].get_cod()));
                }
            }       //revisa Camiones directos
            
            for(unsigned int k=0; k< Edges[j->edge].Camiones2.size(); k++)
            {
                for(unsigned int m=0; m< Edges[j->edge].Camiones2[k].size(); m++)
                {
                    vector<int> IDC;
                    bool Feeder = Find_camion_feeder_interplanta(IDC, Edges[j->edge].Camiones2[k][m], Edges, Nodes);
                    int NDias = ceil(Edges[j->edge].Camiones2[k][m].TTotal/24);
                    
                    for(unsigned int i=0; i<Marcados.size(); i++)
                    {
                        if(Marcados[i] == Edges[j->edge].Camiones2[k][m].get_idc())
                            NDias--;
                    }
                  
                    if((Edges[j->edge].Camiones2[k][m].TTotal < theta) || (NDias == 0))    //todo se programa el mismo día cero
                        Nodes[node_id].Schedule0.push_back(trioProgramacion(Edges[j->edge].Camiones2[k][m].get_idc(), Edges[j->edge].Camiones2[k][m].TTotal,Feeder,Nodes[Edges[j->edge].v()].get_cod()));
                    else if(NDias == 1)             //un día de atraso
                        Nodes[node_id].Schedule1.push_back(trioProgramacion(Edges[j->edge].Camiones2[k][m].get_idc(), Edges[j->edge].Camiones2[k][m].TTotal,Feeder, Nodes[Edges[j->edge].v()].get_cod()));
                    else if(NDias == 2)             //un día de atraso
                        Nodes[node_id].Schedule2.push_back(trioProgramacion(Edges[j->edge].Camiones2[k][m].get_idc(), Edges[j->edge].Camiones2[k][m].TTotal,Feeder, Nodes[Edges[j->edge].v()].get_cod()));
                    else if(NDias >= 3)             //un día de atraso
                        Nodes[node_id].Schedule3.push_back(trioProgramacion(Edges[j->edge].Camiones2[k][m].get_idc(), Edges[j->edge].Camiones2[k][m].TTotal,Feeder, Nodes[Edges[j->edge].v()].get_cod()));
                }
            }       //revisa Camiones2
            
            if(Nodes[Edges[j->edge].v()].get_type() == 2)   //sólo reviso arcos S-S
            {
                for(unsigned int k=0; k< Edges[j->edge].CamionesS.size(); k++)
                {
                    for(unsigned int m=0; m< Edges[j->edge].CamionesS[k].size(); m++)
                    {
                        vector<int> IDC;
                        bool Feeder = Find_camion_feeder_interplanta(IDC, Edges[j->edge].CamionesS[k][m], Edges, Nodes);
                        int NDias = ceil(Edges[j->edge].CamionesS[k][m].TTotal/24);
                        
                        for(unsigned int i=0; i<Marcados.size(); i++)
                        {
                            if(Marcados[i] == Edges[j->edge].CamionesS[k][m].get_idc())
                                NDias--;
                        }
                     
                        if((Edges[j->edge].CamionesS[k][m].TTotal < theta)||(NDias == 0))    //todo se programa el mismo día cero
                            Nodes[node_id].Schedule0.push_back(trioProgramacion(Edges[j->edge].CamionesS[k][m].get_idc(), Edges[j->edge].CamionesS[k][m].TTotal,Feeder, Nodes[Edges[j->edge].v()].get_cod()));
                        else if(NDias == 1)             //un día de atraso
                            Nodes[node_id].Schedule1.push_back(trioProgramacion(Edges[j->edge].CamionesS[k][m].get_idc(), Edges[j->edge].CamionesS[k][m].TTotal,Feeder, Nodes[Edges[j->edge].v()].get_cod()));
                        else if(NDias == 2)             //dos días de atraso
                            Nodes[node_id].Schedule2.push_back(trioProgramacion(Edges[j->edge].CamionesS[k][m].get_idc(), Edges[j->edge].CamionesS[k][m].TTotal,Feeder, Nodes[Edges[j->edge].v()].get_cod()));
                        else if(NDias >= 3)             //tres días de atraso
                            Nodes[node_id].Schedule3.push_back(trioProgramacion(Edges[j->edge].CamionesS[k][m].get_idc(), Edges[j->edge].CamionesS[k][m].TTotal,Feeder, Nodes[Edges[j->edge].v()].get_cod()));
                    }
                }       //revisa CamionesS
            }
            
        }       // end for
        
        // LLENA LOS CAMIONES CON LOS SCHEDULES DEFINITIVOS, PRIMERO ORDENA LOS SCHEDULES SEGÚN TTOTAL DE CAMIONES
        if(Nodes[node_id].Schedule0.size()>0)
            Nodes[node_id].Schedule0 = sortTrioP(Nodes[node_id].Schedule0);
        if(Nodes[node_id].Schedule1.size()>0)
            Nodes[node_id].Schedule1 = sortTrioP(Nodes[node_id].Schedule1);
        if(Nodes[node_id].Schedule2.size()>0)
            Nodes[node_id].Schedule2 = sortTrioP(Nodes[node_id].Schedule2);
        if(Nodes[node_id].Schedule3.size()>0)
            Nodes[node_id].Schedule3 = sortTrioP(Nodes[node_id].Schedule3);
        
        //en cada timing, tengo que programar los camiones repartidos en la TW, de ancho W
        int NCam0 = Nodes[node_id].Schedule0.size();
        if(NCam0 > 1)
        {
            double w = Nodes[node_id].getWidthTW()/(NCam0 - 1);
            for(unsigned int k=0; k< Nodes[node_id].Schedule0.size(); k++)
            {
                double DTime = k*w;
                Nodes[node_id].Schedule0[k].programacion = Nodes[node_id].getProgramacion(DTime);
            }
            //ordeno de acuerdo a las programaciones
            sort(Nodes[node_id].Schedule0.begin(),Nodes[node_id].Schedule0.end(),&CompareTrioP);
            
            
        }
        else if(NCam0 == 1)
        {
            double DTime = Nodes[node_id].getWidthTW()/2;   //un camión, lo pongo en la mitad de la ventana
            Nodes[node_id].Schedule0[0].programacion = Nodes[node_id].getProgramacion(DTime);
        }
        
        int NCam1 = Nodes[node_id].Schedule1.size();
        if(NCam1 > 1)
        {
            double w = Nodes[node_id].getWidthTW()/(NCam1 - 1);
            for(unsigned int k=0; k< Nodes[node_id].Schedule1.size(); k++)
            {
                double DTime = k*w;
                Nodes[node_id].Schedule1[k].programacion = Nodes[node_id].getProgramacion(DTime) + 24*3600;
            }
            //ordeno de acuerdo a las programaciones
            sort(Nodes[node_id].Schedule1.begin(),Nodes[node_id].Schedule1.end(),&CompareTrioP);
        }
        else if(NCam1 == 1)
        {
            double DTime = Nodes[node_id].getWidthTW()/2;   //un camión, lo pongo en la mitad de la ventana
            Nodes[node_id].Schedule1[0].programacion = Nodes[node_id].getProgramacion(DTime) + 24*3600;
        }
            
        int NCam2 = Nodes[node_id].Schedule2.size();
        if(NCam2 > 1)
        {
            double w = Nodes[node_id].getWidthTW()/(NCam2 - 1);
            for(unsigned int k=0; k< Nodes[node_id].Schedule2.size(); k++)
            {
                double DTime = k*w;
                Nodes[node_id].Schedule2[k].programacion = Nodes[node_id].getProgramacion(DTime) + 2*24*3600;
            }
            //ordeno de acuerdo a las programaciones
            sort(Nodes[node_id].Schedule2.begin(),Nodes[node_id].Schedule2.end(),&CompareTrioP);
        }
        else if(NCam2 == 1)
        {
            double DTime = Nodes[node_id].getWidthTW()/2;   //un camión, lo pongo en la mitad de la ventana
            Nodes[node_id].Schedule2[0].programacion = Nodes[node_id].getProgramacion(DTime) + 2*24*3600;
        }
        
        int NCam3 = Nodes[node_id].Schedule3.size();
        if(NCam3 > 1)
        {
            double w = Nodes[node_id].getWidthTW()/(NCam3 - 1);
            for(unsigned int k=0; k< Nodes[node_id].Schedule3.size(); k++)
            {
                double DTime = k*w;
                Nodes[node_id].Schedule3[k].programacion = Nodes[node_id].getProgramacion(DTime) + 3*24*3600;
            }
            //ordeno de acuerdo a las programaciones
            sort(Nodes[node_id].Schedule3.begin(),Nodes[node_id].Schedule3.end(),&CompareTrioP);
        }
        else if(NCam3 == 1)
        {
            double DTime = Nodes[node_id].getWidthTW()/2;   //un camión, lo pongo en la mitad de la ventana
            Nodes[node_id].Schedule3[0].programacion = Nodes[node_id].getProgramacion(DTime) + 3*24*3600;
        }

    }   // en if chequeando que nodo es sucursal

}           // en fn FillCamionesSucursal

void Graph::AjustaItinerarios(vector<Edge>& Edges, vector<Node>& Nodes)
{
    for(unsigned int k=0; k<Nodes.size(); k++)
    {
        if(Nodes[k].get_type() == 2)
        {
            for(unsigned int j=0; j<Nodes[k].Schedule0.size(); j++)
            {
                time_t RedonT = Nodes[k].CorrigeTiempoAsignacion(Nodes[k].Schedule0[j].programacion);
                Nodes[k].Schedule0[j].programacion = RedonT;
            }
            for(unsigned int j=0; j<Nodes[k].Schedule1.size(); j++)
            {
                time_t RedonT = Nodes[k].CorrigeTiempoAsignacion(Nodes[k].Schedule1[j].programacion);
                Nodes[k].Schedule1[j].programacion = RedonT;
            }
            for(unsigned int j=0; j<Nodes[k].Schedule2.size(); j++)
            {
                time_t RedonT = Nodes[k].CorrigeTiempoAsignacion(Nodes[k].Schedule2[j].programacion);
                Nodes[k].Schedule2[j].programacion = RedonT;
            }
            for(unsigned int j=0; j<Nodes[k].Schedule3.size(); j++)
            {
                time_t RedonT = Nodes[k].CorrigeTiempoAsignacion(Nodes[k].Schedule3[j].programacion);
                Nodes[k].Schedule3[j].programacion = RedonT;
            }
        }       // end check nodos de tipo 2
        
    }   // end for recorriendo nodos
}

//dada una programación en Nodos destino final, acá se programa las salidas de planta de tales camiones
void Graph::FillCamionesPlanta(vector<Edge>& Edges, vector<Node>& Nodes)
{
    for(unsigned int i=0; i<Nodes.size();i++)
    {
        if(Nodes[i].get_type() == 2)    //nodo sucursal
        {
            for(unsigned int k=0; k<Nodes[i].Schedule0.size();k++)      //SCHEDULE0 ++++++++++++++++++++++++++
            {
                vector<camion_identificador> ff = Find_camion(Nodes[i].Schedule0[k].camion_id, Edges);
                
                if(ff.size() == 1)
                {
                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule0[k].ttotal, Nodes[i].Schedule0[k].TieneCamionFeeder, Nodes[i].get_cod()));
                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Nodes[i].Schedule0[k].programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                    
                }
                else if(ff.size() == 2)
                {
                    //codición extra en caso de CamionesS
                    if((Edges[ff[0].edge_id].v() != i)&&(Edges[ff[1].edge_id].v() != i))
                    {
                        if(Edges[ff[0].edge_id].w() == i)   // arco en ff[0] es el último (P-S) o (S-S)
                        {
                            Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule0[k].ttotal, Nodes[i].Schedule0[k].TieneCamionFeeder, Nodes[i].get_cod()));
                            Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Nodes[i].Schedule0[k].programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                            
                            Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule0[k].ttotal, Nodes[i].Schedule0[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                            Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                        }
                        else
                        {
                            Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule0[k].ttotal, Nodes[i].Schedule0[k].TieneCamionFeeder, Nodes[i].get_cod()));
                            Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Nodes[i].Schedule0[k].programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                            
                            Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule0[k].ttotal, Nodes[i].Schedule0[k].TieneCamionFeeder,Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                            Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                        }
                        
                    }   // end if descartando caso arcps P-S
                    
                }       //caso dos etapas
                
            }       //fin for schedule 0
            
            for(unsigned int k=0; k<Nodes[i].Schedule1.size();k++)      //SCHEDULE1++++++++++++++++++++++++
            {
                vector<camion_identificador> ff = Find_camion(Nodes[i].Schedule1[k].camion_id, Edges);
                
                if(ff.size() == 1)
                {
                    time_t Programacion = Nodes[i].Schedule1[k].programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                    bool dentroTW;
                    int dia = Nodes[Edges[ff[0].edge_id].v()].DiaProgramacion(Programacion, &dentroTW);
                    if(dia == 0)
                    {
                        Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule1[k].ttotal, Nodes[i].Schedule1[k].TieneCamionFeeder, Nodes[i].get_cod()));
                        Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Programacion;
                    }
                    else if(dia == 1)
                    {
                        Nodes[Edges[ff[0].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule1[k].ttotal, Nodes[i].Schedule1[k].TieneCamionFeeder, Nodes[i].get_cod()));
                        Nodes[Edges[ff[0].edge_id].v()].Schedule1.back().programacion = Programacion;
                    }
                    
                }
                else if(ff.size() == 2)
                {
                    //codición extra en caso de CamionesS
                    if((Edges[ff[0].edge_id].v() != i)&&(Edges[ff[1].edge_id].v() != i))
                    {
                        if(Edges[ff[0].edge_id].w() == i)   // arco en ff[0] es el último (P-S) o (S-S)
                        {
                            time_t Programacion = Nodes[i].Schedule1[k].programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                            bool dentroTW;
                            int dia = Nodes[Edges[ff[0].edge_id].v()].DiaProgramacion(Programacion, &dentroTW);
                            
                            if(dia == 0)
                            {
                                Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule1[k].ttotal, Nodes[i].Schedule1[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Programacion;
                            
                                Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule1[k].ttotal, Nodes[i].Schedule1[k].TieneCamionFeeder,Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                            }
                            else if(dia == 1)
                            {
                                Nodes[Edges[ff[0].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule1[k].ttotal, Nodes[i].Schedule1[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[0].edge_id].v()].Schedule1.back().programacion = Programacion;
                                
                                time_t Programacion2 = Programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                                bool dentroTW;
                                int dia2 = Nodes[Edges[ff[1].edge_id].v()].DiaProgramacion(Programacion2, &dentroTW);
                                
                                if(dia2 == 0)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule1[k].ttotal, Nodes[i].Schedule1[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Programacion2;
                                }
                                else if(dia2 == 1)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule1[k].ttotal, Nodes[i].Schedule1[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule1.back().programacion = Programacion2;
                                }
                            
                            }
                            
                        }       // end if
                        else
                        {
                            time_t Programacion = Nodes[i].Schedule1[k].programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                            bool dentroTW;
                            int dia = Nodes[Edges[ff[1].edge_id].v()].DiaProgramacion(Programacion, &dentroTW);
                            
                            if(dia == 0)
                            {
                                Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule1[k].ttotal, Nodes[i].Schedule1[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Programacion;
                            
                                Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule1[k].ttotal, Nodes[i].Schedule1[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                            }
                            else if(dia == 1)
                            {
                                Nodes[Edges[ff[1].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule1[k].ttotal, Nodes[i].Schedule1[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[1].edge_id].v()].Schedule1.back().programacion = Programacion;
                                
                                time_t Programacion2 = Programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                                bool dentroTW;
                                int dia2 = Nodes[Edges[ff[0].edge_id].v()].DiaProgramacion(Programacion2, &dentroTW);
                                
                                if(dia2 == 0)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule1[k].ttotal, Nodes[i].Schedule1[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Programacion2;
                                }
                                else if(dia2 == 1)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule1[k].ttotal, Nodes[i].Schedule1[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule1.back().programacion = Programacion2;
                                }
                            
                            }
                            
                        }       // end if
                        
                    }   // end if descartando caso arcps P-S
                    
                }       //caso dos etapas
                
            }       //fin for schedule 1
            
            for(unsigned int k=0; k<Nodes[i].Schedule2.size();k++)      //SCHEDULE2++++++++++++++++++++++++
            {
                vector<camion_identificador> ff = Find_camion(Nodes[i].Schedule2[k].camion_id, Edges);
                
                if(ff.size() == 1)
                {
                    time_t Programacion = Nodes[i].Schedule2[k].programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                    bool dentroTW;
                    int dia = Nodes[Edges[ff[0].edge_id].v()].DiaProgramacion(Programacion, &dentroTW);
                    if(dia == 0)
                    {
                        Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[i].get_cod()));
                        Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Programacion;
                    }
                    else if(dia == 1)
                    {
                        Nodes[Edges[ff[0].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[i].get_cod()));
                        Nodes[Edges[ff[0].edge_id].v()].Schedule1.back().programacion = Programacion;
                    }
                    else if(dia == 2)
                    {
                        Nodes[Edges[ff[0].edge_id].v()].Schedule2.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[i].get_cod()));
                        Nodes[Edges[ff[0].edge_id].v()].Schedule2.back().programacion = Programacion;
                    }
                    
                }
                else if(ff.size() == 2)
                {
                    //codición extra en caso de CamionesS
                    if((Edges[ff[0].edge_id].v() != i)&&(Edges[ff[1].edge_id].v() != i))
                    {
                        if(Edges[ff[0].edge_id].w() == i)   // arco en ff[0] es el último (P-S) o (S-S)
                        {
                            time_t Programacion = Nodes[i].Schedule2[k].programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                            bool dentroTW;
                            int dia = Nodes[Edges[ff[0].edge_id].v()].DiaProgramacion(Programacion, &dentroTW);
                            
                            if(dia == 0)
                            {
                                Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Programacion;
                            
                                Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder,Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                            }
                            else if(dia == 1)
                            {
                                Nodes[Edges[ff[0].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[0].edge_id].v()].Schedule1.back().programacion = Programacion;
                                
                                time_t Programacion2 = Programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                                bool dentroTW;
                                int dia2 = Nodes[Edges[ff[1].edge_id].v()].DiaProgramacion(Programacion2, &dentroTW);
                                
                                if(dia2 == 0)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Programacion2;
                                }
                                else if(dia2 == 1)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule1.back().programacion = Programacion2;
                                }
                            
                            }
                            
                            else if(dia == 2)
                            {
                                Nodes[Edges[ff[0].edge_id].v()].Schedule2.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[0].edge_id].v()].Schedule2.back().programacion = Programacion;
                                
                                time_t Programacion2 = Programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                                bool dentroTW;
                                int dia2 = Nodes[Edges[ff[1].edge_id].v()].DiaProgramacion(Programacion2, &dentroTW);
                                
                                if(dia2 == 0)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Programacion2;
                                }
                                else if(dia2 == 1)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule1.back().programacion = Programacion2;
                                }
                                else if(dia2 == 2)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule2.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule2.back().programacion = Programacion2;
                                }
                            
                            }
                            
                        }       // end if
                        else
                        {
                            time_t Programacion = Nodes[i].Schedule2[k].programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                            bool dentroTW;
                            int dia = Nodes[Edges[ff[1].edge_id].v()].DiaProgramacion(Programacion, &dentroTW);
                            
                            if(dia == 0)
                            {
                                Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Programacion;
                            
                                Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                            }
                            else if(dia == 1)
                            {
                                Nodes[Edges[ff[1].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[1].edge_id].v()].Schedule1.back().programacion = Programacion;
                                
                                time_t Programacion2 = Programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                                bool dentroTW;
                                int dia2 = Nodes[Edges[ff[0].edge_id].v()].DiaProgramacion(Programacion2, &dentroTW);
                                
                                if(dia2 == 0)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Programacion2;
                                }
                                else if(dia2 == 1)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule1.back().programacion = Programacion2;
                                }
                            
                            }
                            
                            else if(dia == 2)
                            {
                                Nodes[Edges[ff[1].edge_id].v()].Schedule2.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[1].edge_id].v()].Schedule2.back().programacion = Programacion;
                                
                                time_t Programacion2 = Programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                                bool dentroTW;
                                int dia2 = Nodes[Edges[ff[0].edge_id].v()].DiaProgramacion(Programacion2, &dentroTW);
                                
                                if(dia2 == 0)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Programacion2;
                                }
                                else if(dia2 == 1)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule1.back().programacion = Programacion2;
                                }
                                else if(dia2 == 2)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule2.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule2[k].ttotal, Nodes[i].Schedule2[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule2.back().programacion = Programacion2;
                                }
                            
                            }
                            
                        }       // end if
                        
                    }   // end if descartando caso arcps P-S
                    
                }       //caso dos etapas
                
            }       //fin for schedule 2
            
            for(unsigned int k=0; k<Nodes[i].Schedule3.size();k++)      //SCHEDULE3++++++++++++++++++++++++
            {
                vector<camion_identificador> ff = Find_camion(Nodes[i].Schedule3[k].camion_id, Edges);
                
                if(ff.size() == 1)
                {
                    time_t Programacion = Nodes[i].Schedule3[k].programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                    bool dentroTW;
                    int dia = Nodes[Edges[ff[0].edge_id].v()].DiaProgramacion(Programacion, &dentroTW);
                    if(dia == 0)
                    {
                        Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[i].get_cod()));
                        Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Programacion;
                    }
                    else if(dia == 1)
                    {
                        Nodes[Edges[ff[0].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[i].get_cod()));
                        Nodes[Edges[ff[0].edge_id].v()].Schedule1.back().programacion = Programacion;
                    }
                    else if(dia == 2)
                    {
                        Nodes[Edges[ff[0].edge_id].v()].Schedule2.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[i].get_cod()));
                        Nodes[Edges[ff[0].edge_id].v()].Schedule2.back().programacion = Programacion;
                    }
                    else if(dia == 3)
                    {
                        Nodes[Edges[ff[0].edge_id].v()].Schedule3.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[i].get_cod()));
                        Nodes[Edges[ff[0].edge_id].v()].Schedule3.back().programacion = Programacion;
                    }
                    
                }
                else if(ff.size() == 2)
                {
                    //codición extra en caso de CamionesS
                    if((Edges[ff[0].edge_id].v() != i)&&(Edges[ff[1].edge_id].v() != i))
                    {
                        if(Edges[ff[0].edge_id].w() == i)   // arco en ff[0] es el último (P-S) o (S-S)
                        {
                            time_t Programacion = Nodes[i].Schedule3[k].programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                            bool dentroTW;
                            int dia = Nodes[Edges[ff[0].edge_id].v()].DiaProgramacion(Programacion, &dentroTW);
                            
                            if(dia == 0)
                            {
                                Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Programacion;
                            
                                Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder,Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                            }
                            else if(dia == 1)
                            {
                                Nodes[Edges[ff[0].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[0].edge_id].v()].Schedule1.back().programacion = Programacion;
                                
                                time_t Programacion2 = Programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                                bool dentroTW;
                                int dia2 = Nodes[Edges[ff[1].edge_id].v()].DiaProgramacion(Programacion2, &dentroTW);
                                
                                if(dia2 == 0)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Programacion2;
                                }
                                else if(dia2 == 1)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule1.back().programacion = Programacion2;
                                }
                            
                            }
                            
                            else if(dia == 2)
                            {
                                Nodes[Edges[ff[0].edge_id].v()].Schedule2.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[0].edge_id].v()].Schedule2.back().programacion = Programacion;
                                
                                time_t Programacion2 = Programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                                bool dentroTW;
                                int dia2 = Nodes[Edges[ff[1].edge_id].v()].DiaProgramacion(Programacion2, &dentroTW);
                                
                                if(dia2 == 0)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Programacion2;
                                }
                                else if(dia2 == 1)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder,Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule1.back().programacion = Programacion2;
                                }
                                else if(dia2 == 2)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule2.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule2.back().programacion = Programacion2;
                                }
                            
                            }
                            
                            else if(dia == 3)
                            {
                                Nodes[Edges[ff[0].edge_id].v()].Schedule3.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[0].edge_id].v()].Schedule3.back().programacion = Programacion;
                                
                                time_t Programacion2 = Programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                                bool dentroTW;
                                int dia2 = Nodes[Edges[ff[1].edge_id].v()].DiaProgramacion(Programacion2, &dentroTW);
                                
                                if(dia2 == 0)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Programacion2;
                                }
                                else if(dia2 == 1)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule1.back().programacion = Programacion2;
                                }
                                else if(dia2 == 2)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule2.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule2.back().programacion = Programacion2;
                                }
                                else if(dia2 == 3)
                                {
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule3.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[0].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[1].edge_id].v()].Schedule3.back().programacion = Programacion2;
                                }
                                
                            }
                            
                        }       // end if
                        
                        else
                        {
                            time_t Programacion = Nodes[i].Schedule3[k].programacion - (Edges[ff[1].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                            bool dentroTW;
                            int dia = Nodes[Edges[ff[1].edge_id].v()].DiaProgramacion(Programacion, &dentroTW);
                            
                            if(dia == 0)
                            {
                                Nodes[Edges[ff[1].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[1].edge_id].v()].Schedule0.back().programacion = Programacion;
                            
                                Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[1].edge_id].v()].getTcarga())*3600;
                            }
                            else if(dia == 1)
                            {
                                Nodes[Edges[ff[1].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[1].edge_id].v()].Schedule1.back().programacion = Programacion;
                                
                                time_t Programacion2 = Programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                                bool dentroTW;
                                int dia2 = Nodes[Edges[ff[0].edge_id].v()].DiaProgramacion(Programacion2, &dentroTW);
                                
                                if(dia2 == 0)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Programacion2;
                                }
                                else if(dia2 == 1)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule1.back().programacion = Programacion2;
                                }
                            }
                            
                            else if(dia == 2)
                            {
                                Nodes[Edges[ff[1].edge_id].v()].Schedule2.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[1].edge_id].v()].Schedule2.back().programacion = Programacion;
                                
                                time_t Programacion2 = Programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                                bool dentroTW;
                                int dia2 = Nodes[Edges[ff[0].edge_id].v()].DiaProgramacion(Programacion2, &dentroTW);
                                
                                if(dia2 == 0)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Programacion2;
                                }
                                else if(dia2 == 1)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule1.back().programacion = Programacion2;
                                }
                                else if(dia2 == 2)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule2.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule2.back().programacion = Programacion2;
                                }
                            }
                            
                            else if(dia == 3)
                            {
                                Nodes[Edges[ff[1].edge_id].v()].Schedule3.push_back(trioProgramacion(ff[1].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[i].get_cod()));
                                Nodes[Edges[ff[1].edge_id].v()].Schedule3.back().programacion = Programacion;
                                
                                time_t Programacion2 = Programacion - (Edges[ff[0].edge_id].getTViaje() + Nodes[Edges[ff[0].edge_id].v()].getTcarga())*3600;
                                bool dentroTW;
                                int dia2 = Nodes[Edges[ff[0].edge_id].v()].DiaProgramacion(Programacion2, &dentroTW);
                                
                                if(dia2 == 0)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule0.back().programacion = Programacion2;
                                }
                                else if(dia2 == 1)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule1.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule1.back().programacion = Programacion2;
                                }
                                else if(dia2 == 2)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule2.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule2.back().programacion = Programacion2;
                                }
                                else if(dia2 == 3)
                                {
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule3.push_back(trioProgramacion(ff[0].camion_id, Nodes[i].Schedule3[k].ttotal, Nodes[i].Schedule3[k].TieneCamionFeeder, Nodes[Edges[ff[1].edge_id].v()].get_cod()));
                                    Nodes[Edges[ff[0].edge_id].v()].Schedule3.back().programacion = Programacion2;
                                }

                            }
                            
                        }       // end if
                        
                    }   // end if descartando caso arcps P-S
                    
                }       //caso dos etapas
                
            }       //fin for schedule 3
            
            
        }       // END IF NODES TYPE 2
        
            //REPETIR PARA TODOS LOS SCHEDULES
    }       //analizando a partir de sucursales ya programadas
}


struct restrInterplanta
{
    int Cam_id;
    time_t Constraint;
    int nodo_programacion;
    restrInterplanta(int cid, time_t Pr, int np): Cam_id(cid),Constraint(Pr), nodo_programacion(np) {}
};

int Graph::FindEdgeFeeder(int node_id, int camion_id, int* pos, vector<Edge>& Edges)
{
    list<pairCC>::iterator i;
    for (i = adjb[node_id].begin(); i != adjb[node_id].end(); ++i)
    {
        for(unsigned int j=0; j< Edges[i->edge].CamionesD.size(); j++)
        {
            if(Edges[i->edge].CamionesD[j].get_idc() == camion_id)
            {
                *pos = j;
                return(i->edge);
            }
        }
    }
    return(-1);
}

Camion Graph::FindCamionSaliente(int node_id, int camion_id, vector<Edge>& Edges)
{
    list<pairCC>::iterator i;
    for (i = adj[node_id].begin(); i != adj[node_id].end(); ++i)
    {
        for(unsigned int j=0; j< Edges[i->edge].CamionesD.size(); j++)
        {
            if(camion_id == Edges[i->edge].CamionesD[j].get_idc())
                return(Edges[i->edge].CamionesD[j]);
        }
        for(unsigned int j=0; j< Edges[i->edge].Camiones2.size(); j++)
        {
            for(unsigned int k=0; k< Edges[i->edge].Camiones2[j].size(); k++)
            {
                if(camion_id == Edges[i->edge].Camiones2[j][k].get_idc())
                    return(Edges[i->edge].Camiones2[j][k]);
            }
            
        }
        for(unsigned int j=0; j< Edges[i->edge].CamionesS.size(); j++)
        {
            for(unsigned int k=0; k< Edges[i->edge].CamionesS[j].size(); k++)
            {
                if(camion_id == Edges[i->edge].CamionesS[j][k].get_idc())
                    return(Edges[i->edge].CamionesS[j][k]);
            }
            
        }
    }   // end for
    Camion Dummy(-1000);
    return(Dummy);
}       // end función

// finalmente programo los camiones feeder
void Graph::FillCamionesFeeder(vector<Edge>& Edges, vector<Node>& Nodes)
{
    vector<restrInterplanta> Feeders;
    for(unsigned int k=0; k<Nodes.size(); k++)
    {
        if(Nodes[k].get_type() == 1)    //nodo planta, acá debe sincronizarse los feeders
        {
            for(unsigned int pp=0; pp<Nodes[k].Schedule0.size(); pp++)
            {
                vector<int> Cam_id;
                bool tieneFeeder = Find_camion_feeder_interplanta(Cam_id, FindCamionSaliente(k, Nodes[k].Schedule0[pp].camion_id, Edges), Edges, Nodes);
                
                if(tieneFeeder)
                {
                    bool cond = false;
                    for(unsigned int m=0; m < Cam_id.size(); m++)
                    {
                        for(unsigned int l=0; l< Feeders.size(); l++)
                        {
                            if(Cam_id[m] == Feeders[l].Cam_id)
                            {
                                cond = true;
                                if(Nodes[k].Schedule0[pp].programacion < Feeders[l].Constraint)
                                    Feeders[l].Constraint = Nodes[k].Schedule0[pp].programacion;
                            }
                        }       // end for
                        if(!cond)
                            Feeders.push_back(restrInterplanta(Cam_id[m],Nodes[k].Schedule0[pp].programacion,k));
                    }           // end for
                }
            }   // end for schedule0
            
        }   // end if recorriendo plantas
    }       // end for chequeando nodos
    
    // ahora procedemos a programar estos camiones interplanta
    for(unsigned int i=0; i< Feeders.size(); i++)
    {
        int posD;
        int edgeF = FindEdgeFeeder(Feeders[i].nodo_programacion, Feeders[i].Cam_id, &posD, Edges);
        
        if(edgeF != -1)
        {
            double Ttotal = Edges[edgeF].CamionesD[posD].TTotal;
            Nodes[Edges[edgeF].w()].Schedule0.push_back(trioProgramacion(Feeders[i].Cam_id, Ttotal, 0, Nodes[Edges[edgeF].v()].get_cod()));
            Nodes[Edges[edgeF].w()].Schedule0.back().programacion = Feeders[i].nodo_programacion - (Nodes[Edges[edgeF].w()].getTcarga() + Nodes[Edges[edgeF].w()].getTdescarga())*3600;
            
            Nodes[Edges[edgeF].v()].Schedule0.push_back(trioProgramacion(Feeders[i].Cam_id, Ttotal, 0, Nodes[Edges[edgeF].w()].get_cod()));
            Nodes[Edges[edgeF].v()].Schedule0.back().programacion = Nodes[Edges[edgeF].w()].Schedule0.back().programacion - (Edges[edgeF].getTViaje() + Nodes[Edges[edgeF].v()].getTcarga())*3600;
        }
    }
}           // end función fillCamionesFeeders

string Graph::PrintLabels1(vector<Edge>& Edges, vector<Node>& Nodes)
{
    stringstream s;
    s << "Graph N: " << Nodes.size() << endl;
    int kk = 0;
    int pp = 0;
    for(unsigned int v=0; v < V; v++)
    {
        list<pairCC>::iterator i;
        for (i = adj[v].begin(); i != adj[v].end(); ++i)
        {
            if(Edges[i->edge].ArcUsed())
            {
                s << "E:" << i->edge << "  TV: " << Edges[i->edge].getTViaje() << " NO: " << Nodes[Edges[i->edge].v()].get_cod() << " ND: " << Nodes[Edges[i->edge].w()].get_cod() << " xsize: " << Edges[i->edge].x.size() << " xssize: " << Edges[i->edge].xs.size() << " x2size: " << Edges[i->edge].x2.size() << " xrsize: " << Edges[i->edge].xr.size() << " ysize: " << Edges[i->edge].y.size() << " y2size: " << Edges[i->edge].y2.size() << " yssize: " << Edges[i->edge].ys.size() << " psize: " << Edges[i->edge].p.size() << " p2size: " << Edges[i->edge].p2.size() << " prsize: " << Edges[i->edge].pr.size() << " zsize: " << Edges[i->edge].z.size() << " z2size: " << Edges[i->edge].z2.size() << " zrsize: " << Edges[i->edge].zr.size() << " zssize: " << Edges[i->edge].zs.size() <<endl;
                pp++;
            }
            kk++;
        }
    }
    s << "Numero de arcos: " << kk << " usados: " << pp << endl;
    s << endl;
    s << "Red backwards" << endl;
    kk = 0;
    pp = 0;
    for(unsigned int w=0; w < V; w++)
    {
        list<pairCC>::iterator i;
        for (i = adjb[w].begin(); i != adjb[w].end(); ++i)
        {
            if(Edges[i->edge].ArcUsed())
            {
                s << "E:" << i->edge << "  NO: " << Nodes[Edges[i->edge].v()].get_cod() << " ND: " << Nodes[Edges[i->edge].w()].get_cod() << " xsize: " << Edges[i->edge].x.size() << " xssize: " << Edges[i->edge].xs.size() << " x2size: " << Edges[i->edge].x2.size() << " xrsize: " << Edges[i->edge].xr.size() << " ysize: " << Edges[i->edge].y.size() << " y2size: " << Edges[i->edge].y2.size() << " yssize: " << Edges[i->edge].ys.size() << " psize: " << Edges[i->edge].p.size() << " p2size: " << Edges[i->edge].p2.size() << " prsize: " << Edges[i->edge].pr.size() << " zsize: " << Edges[i->edge].z.size() << " z2size: " << Edges[i->edge].z2.size() << " zrsize: " << Edges[i->edge].zr.size() << " zssize: " << Edges[i->edge].zs.size() <<endl;
                pp++;
            }
            kk++;
        }
    }
    s << "Numero de arcos: " << kk << " usados: " << pp << endl;
    return s.str();
}

// imprime los labels usando la estructura Edges
string Graph::PrintLabels(vector<Edge>& Edges, vector<Node>& Nodes)
{
    stringstream s;
    s << "Graph N: " << Nodes.size() << " A: " << Edges.size() << endl;
    
    for(unsigned  int k= 0; k<Edges.size(); k++)
    {
        if(Edges[k].ArcUsed())
        {
            s << "E:" << k << "  NO: " << Nodes[Edges[k].v()].get_cod() << " ND: " << Nodes[Edges[k].w()].get_cod() << " xsize: " << Edges[k].x.size() << " x2size: " << Edges[k].x2.size() << " xrsize: " << Edges[k].xr.size() << " xssize: " << Edges[k].xs.size() << " ysize: " << Edges[k].y.size() << " y2size: " << Edges[k].y2.size() << " yssize: " << Edges[k].ys.size() << " psize: " << Edges[k].p.size() << " p2size: " << Edges[k].p2.size() << " prsize: " << Edges[k].pr.size() << " zsize: " << Edges[k].z.size() << " z2size: " << Edges[k].z2.size() << " zrsize: " << Edges[k].zr.size() << " zssize: " << Edges[k].zs.size() << endl;
        }
    }
    return s.str();
}

struct RutaPrint
{
    string Secuencia;
    vector<vector<double>> carga1;       //guarda la carga en planta 1 por [estado,sector]
    vector<vector<double>> carga2;       // IDEM PLARA PLANTA EN SEGUNDA CAPA
    string tipo_camion;
    int camion_id;                      //camion ID único
    string planta1;
    string planta2;
    double costo0;
    double costo1;
    string ClasifCamion;      // interplanta o primario
    
    RutaPrint(string sec, string tipoc, int cam_id, size_t nsec, size_t nest, string clasifc): Secuencia(sec), tipo_camion(tipoc), camion_id(cam_id), ClasifCamion(clasifc)
    {
        planta1 = "-1";
        planta2 = "-1";
        costo0 = 0;
        costo1 = 0;
        carga1.clear();
        carga2.clear();
        carga1.resize(carga1.size() + nest);
        carga2.resize(carga2.size() + nest);
        for(unsigned int k=0; k<nest; k++)
        {
            for(unsigned int l=0; l<nsec; l++)
            {
                carga1[k].push_back(0.0);
                carga2[k].push_back(0.0);
            }
        }
    }       //termina el constructor de esta estructura
    // actualiza la secuencia cuando encuentra el siguiente arco
    void actualiza_secuencia(string lastPart)
    { Secuencia = Secuencia + lastPart;}
    bool separador()        // indica si tiene separador o no el camión
    {
        vector<int> sep;
        for(unsigned int i=0; i<carga1.size(); i++)
        {
            sep.push_back(0);
            for(unsigned int j=0; j<carga1[i].size(); j++)
            {
                if(carga1[i][j] > 0)
                    sep.back() = 1;
            }
        }
        vector<int> sep2;
        for(unsigned int i=0; i<carga2.size(); i++)
        {
            sep2.push_back(0);
            for(unsigned int j=0; j<carga2[i].size(); j++)
            {
                if(carga2[i][j] > 0)
                    sep2.back() = 1;
            }
        }
        // caso particular refrigerado y congelado solamente
        if((sep[0]+sep2[0] >= 1)&&(sep[1]+sep2[1] >= 1))
            return(true);
        else
            return(false);
    }   //end separador
    
    bool check_carga(int planta)    //chequea si hay carga ya sea en planta1 o planta2
    {
        if(planta == 1)
        {
            for(unsigned int i=0; i<carga1.size(); i++)
            {
                for(unsigned int j=0; j<carga1[i].size(); j++)
                {
                    if(carga1[i][j] > 0)
                        return(true);
                }
            }
        }       // end if
        else if(planta == 2)
        {
            for(unsigned int i=0; i<carga2.size(); i++)
            {
                for(unsigned int j=0; j<carga2[i].size(); j++)
                {
                    if(carga2[i][j] > 0)
                        return(true);
                }
            }
        }
        return(false);
    }   // end fn
    
    //encuentra el id del edge que parte en p en secuencia
    int Find_Edge(vector<Edge>& Edges, vector<Node>& Nodes, string p)
    {
        vector<string> row;
        string topic;
        istringstream s(Secuencia);
        while (getline(s, topic, '-'))
            row.push_back(topic);
        
        if(row.size()>1)
        {
            for(unsigned int k=0; k<(row.size()-1); k++)
            {
                if(row[k] == p)
                {
                    string Nf = row[k+1];
                    int edge_id = get_Edge_id(p,Nf,Edges,Nodes);
                    return(edge_id);
                }
                
            }   //end for
        }
        return(-1);     //no lo encontró...raro sería
    }
};

vector<camion_identificador> Graph::Find_camion(int camion_id, vector<Edge>& Edges)
{
    vector<camion_identificador> CI;
    CI.clear();
    for(unsigned int k=0; k<Edges.size(); k++)
    {
        for(unsigned int j=0; j<Edges[k].CamionesD.size(); j++)
        {
            if(Edges[k].CamionesD[j].get_idc() == camion_id)
            {
                CI.push_back(camion_identificador(camion_id, k, 1, j,-1));
                return(CI);
            }
        }   // end for CamionesD
        
        for(unsigned int m=0; m<Edges[k].Camiones2.size(); m++)
        {
            for(unsigned int l=0; l<Edges[k].Camiones2[m].size(); l++)
            {
                if(Edges[k].Camiones2[m][l].get_idc() == camion_id)
                    CI.push_back(camion_identificador(camion_id, k, 2, m, l));
            }
        }       // end doble for para Camiones2
        
        for(unsigned int m=0; m<Edges[k].CamionesS.size(); m++)
        {
            for(unsigned int l=0; l<Edges[k].CamionesS[m].size(); l++)
            {
                if(Edges[k].CamionesS[m][l].get_idc() == camion_id)
                    CI.push_back(camion_identificador(camion_id, k, 3, m, l));
            }
        }       // end doble for para CamionesS
        
    }
    return(CI);
}                   // end función que identifica arcos de cada camión

//id en vector de Nodes de planta de carga 1 de este camión
int Graph::NodeID_planta1(int camion_id, vector<Edge>& Edges, vector<Node>& Nodes)
{
    vector<camion_identificador> mmm = Find_camion(camion_id, Edges);
    if(mmm.size()==1)
        return(Edges[mmm[0].edge_id].v());
    else if(mmm.size()==2)
    {
        if(Edges[mmm[0].edge_id].w() == Edges[mmm[1].edge_id].v())
            return(Edges[mmm[0].edge_id].v());
        else if(Edges[mmm[1].edge_id].w() == Edges[mmm[0].edge_id].v())
            return(Edges[mmm[1].edge_id].v());
    }
    return(-1);
}                   //NodeID_planta1

vector<int> Graph::NodesID(int camion_id, vector<Edge>& Edges, vector<Node>& Nodes)
{
    vector<int> Aux;
    Aux.clear();
    vector<camion_identificador> mmm = Find_camion(camion_id, Edges);
    if(mmm.size()==1)
    {
        Aux.push_back(Edges[mmm[0].edge_id].v());
        Aux.push_back(Edges[mmm[0].edge_id].w());
    }
    else if(mmm.size()==2)
    {
        if(Edges[mmm[0].edge_id].w() == Edges[mmm[1].edge_id].v())
        {
            Aux.push_back(Edges[mmm[0].edge_id].v());
            Aux.push_back(Edges[mmm[0].edge_id].w());
            Aux.push_back(Edges[mmm[1].edge_id].w());
        }
        else if(Edges[mmm[1].edge_id].w() == Edges[mmm[0].edge_id].v())
        {
            Aux.push_back(Edges[mmm[1].edge_id].v());
            Aux.push_back(Edges[mmm[1].edge_id].w());
            Aux.push_back(Edges[mmm[0].edge_id].w());
        }
    }
    return(Aux);
}

//imprime la ruta completa de in camión en particular
string Graph::PrintRutaCamion(int camion_id, vector<Edge>& Edges, vector<Node>& Nodes)
{
    stringstream s;
    vector<int> NodesSeq = NodesID(camion_id,Edges,Nodes);
    for(unsigned int i=0; i<NodesSeq.size(); i++)
    {
        s << Nodes[NodesSeq[i]].get_cod();
        if(i < (NodesSeq.size()-1))
           s << "-";
    }
    return s.str();
}

string Graph::GetInOutCapacidades(int node_id, vector<Edge>& Edges, vector<Node>& Nodes, vector<Producto>& Prod, vector<Familia>& FA)
{
    stringstream s;
    
    vector<InfoCapac> DataCapacidades;
    DataCapacidades.clear();
    
    //arcos saliendo desde node_id
    list<pairCC>::iterator i;
    for (i = adj[node_id].begin(); i != adj[node_id].end(); ++i)
    {
        
        vector<InfoCapac> AC = Edges[i->edge].DetalleAnalisisCapacidad(true, Nodes, Prod, FA);
        DataCapacidades.insert(DataCapacidades.end(), AC.begin(), AC.end());
    }
    
    list<pairCC>::iterator j;
    for (j = adjb[node_id].begin(); j != adjb[node_id].end(); ++j)
    {
        vector<InfoCapac> AD = Edges[j->edge].DetalleAnalisisCapacidad(false, Nodes, Prod, FA);
        DataCapacidades.insert(DataCapacidades.end(), AD.begin(), AD.end());
    }
    
    //se imprimen líneas de carga y descarga correspondientes a este nodo
    for(unsigned int k=0; k<DataCapacidades.size(); k++)
    {
        s << DataCapacidades[k].camion_id << ";" << DataCapacidades[k].stype << ";" << DataCapacidades[k].TipoViaje << ";" << DataCapacidades[k].nodoOperacion << ";";
        s << PrintRutaCamion(DataCapacidades[k].camion_id, Edges,Nodes) << ";";
        
        double TCD = -1;
        if(DataCapacidades[k].tipoOperacion == "carga")
            TCD = Nodes[node_id].getTcarga();
        else if(DataCapacidades[k].tipoOperacion == "descarga")
            TCD = Nodes[node_id].getTdescarga();
        int pos;
        int sch = Nodes[node_id].findCamionSchedule(DataCapacidades[k].camion_id, &pos);
        time_t tinicio = 0;
        time_t tfinal = 0;
        if(sch == 0)
            tinicio = Nodes[node_id].Schedule0[pos].programacion;
        else if(sch == 1)
            tinicio = Nodes[node_id].Schedule1[pos].programacion;
        else if(sch == 2)
            tinicio = Nodes[node_id].Schedule2[pos].programacion;
        else if(sch == 3)
            tinicio = Nodes[node_id].Schedule3[pos].programacion;
   
        tfinal = tinicio + TCD*3600;
        
        s << tinicio << ";" << tfinal << ";";
        if(DataCapacidades[k].separador)
            s << "S;";
        else
            s << ";";
        if(DataCapacidades[k].compartido)
            s << "C;";
        else
            s << ";";
        s << DataCapacidades[k].sector << ";" << DataCapacidades[k].estado << ";" << DataCapacidades[k].npallets << ";" << DataCapacidades[k].nkilos << ";" << DataCapacidades[k].tipoOperacion << ";" << endl;
        
    }   // end for
    
    return s.str();
}

//cambia los atributos de los camiones dentro de los ejes
void Graph::Actualiza_tiempos_totales(vector<Edge>& Edges, vector<Node>& Nodes, int IdLastCamion)
{
    //recorriendo todos los camiones
    for(unsigned int j=1;j<=IdLastCamion; j++)
    {
        vector<camion_identificador> ff = Find_camion(j,Edges);
        if(ff.size() == 1)
        {
            if(ff[0].CamArray_type != 1)
            {
                cout << "Error dentro de Actualiza_tiempos_totales" << endl;
                exit(1);
            }
            int edge_id = ff[0].edge_id;
            int CamD_id = ff[0].CamArray_id;
            Edges[edge_id].CamionesD[CamD_id].TTotal = Nodes[Edges[edge_id].v()].getTcarga() + Edges[edge_id].getTViaje();
            if(Edges[edge_id].CamionesD[CamD_id].ClasifCamion == "interplanta")
                Edges[edge_id].CamionesD[CamD_id].TTotal += Nodes[Edges[edge_id].w()].getTdescarga();
        }
        else if(ff.size() == 2)
        {
            if(ff[0].CamArray_type == 2)    // en Camiones2
            {
                int edge_ida = ff[0].edge_id;
                int edge_idb = ff[1].edge_id;
                int Cam2_ida = ff[0].CamArray_id;
                int Cam2_id2a = ff[0].CamArray_id2;
                int Cam2_idb = ff[1].CamArray_id;
                int Cam2_id2b = ff[1].CamArray_id2;
                
                double TT = Nodes[Edges[edge_ida].v()].getTcarga() + Nodes[Edges[edge_idb].v()].getTcarga() + Edges[edge_ida].getTViaje() + Edges[edge_idb].getTViaje();
                Edges[edge_ida].Camiones2[Cam2_ida][Cam2_id2a].TTotal = TT;
                Edges[edge_idb].Camiones2[Cam2_idb][Cam2_id2b].TTotal = TT;
            }
            else if(ff[0].CamArray_type == 3)   // en CamionesS
            {
                int edge_ida = ff[0].edge_id;
                int edge_idb = ff[1].edge_id;
                int CamS_ida = ff[0].CamArray_id;
                int CamS_id2a = ff[0].CamArray_id2;
                int CamS_idb = ff[1].CamArray_id;
                int CamS_id2b = ff[1].CamArray_id2;
                
                double TT = Edges[edge_ida].getTViaje() + Edges[edge_idb].getTViaje();
                if(Nodes[Edges[edge_ida].v()].get_type() == 1)
                    TT += Nodes[Edges[edge_ida].v()].getTcarga() + Nodes[Edges[edge_idb].v()].getTdescarga();
                else
                    TT += Nodes[Edges[edge_ida].v()].getTdescarga() + Nodes[Edges[edge_idb].v()].getTcarga();
                
                Edges[edge_ida].CamionesS[CamS_ida][CamS_id2a].TTotal = TT;
                Edges[edge_idb].CamionesS[CamS_idb][CamS_id2b].TTotal = TT;
            }
        }       // end else chequeando size =2 camion viajando en dos arcos
            
    }           // end for
    
}   // end fn Actualiza Tiempos Totales

// busca y entrega el ID de todos los camiones de consolidación que elimentan a Camion CM
bool Graph::Find_camion_feeder_interplanta(vector<int>& IDcamiones, Camion CM, vector<Edge>& Edges, vector<Node>& Nodes)
{
    bool cond = false;
    IDcamiones.clear();
    vector<int>::iterator it;
    if(CM.carga.size()>0)
    {
        for(unsigned int k=0; k<CM.carga.size(); k++)
        {
            int id_pallet = CM.carga[k].getid_pallet();
            for(unsigned int i=0; i<Edges.size(); i++)
            {
                if((Nodes[Edges[i].v()].get_type() == 1) && (Nodes[Edges[i].w()].get_type() == 1))
                {
                    if(Edges[i].CamionesD.size()>0)     //potenciales camiones interplanta
                    {
                        for(unsigned int j=0; j<Edges[i].CamionesD.size(); j++)
                        {
                            if(Edges[i].CamionesD[j].carga.size()>0)
                            {
                                for(unsigned int m=0; m<Edges[i].CamionesD[j].carga.size(); m++)
                                {
                                    if(Edges[i].CamionesD[j].carga[m].getid_pallet() == id_pallet)  //encontró camción interplanta suministrando a este camión
                                    {
                                        it = find(IDcamiones.begin(),IDcamiones.end(),Edges[i].CamionesD[j].get_idc());
                                        if((it == IDcamiones.end()) && (Edges[i].CamionesD[j].get_idc() != CM.get_idc()) && (id_pallet != -100))
                                        {
                                            IDcamiones.push_back(Edges[i].CamionesD[j].get_idc());
                                            cond = true;
                                        }
                                    }
                                }   //end for chequeando caga de camiones
                            }
                        }           // end for camiones directos interplanta
                    }
                }
                
            }   // end for recorriendo edges
            
        }   // end for recorriendo palleets en CM
        
    }   // end chequeando si tengo carga o no
    
    if(IDcamiones.size()>0)
        sort(IDcamiones.begin(),IDcamiones.end());
    
   return(cond);         //false si no encuentra camión alimentador
}

//función que llena vector de cajas de este producto en en matriz de origenes, primera fila no supermercado, segunda fila si supermercado, columnas identifican plantas de origen, primario es false viaje es interplanta
vector<vector<int>> Graph::IdentificaOrigen(string CodProducto, string ND, set<string>& Plantas, vector<Edge>& Edges, vector<Node>& Nodes, vector<Producto>& Prod, bool* primario)
{
    
    //inicializa matriz de cajas
    vector<vector<int>> Ncajas;
    Ncajas.resize(Ncajas.size() + 2);
    for(unsigned int i=0; i<Plantas.size(); i++)
    {
        Ncajas[0].push_back(0);
        Ncajas[1].push_back(0);
    }
    
    int node_id = get_Node_id(ND,Nodes);
    // identificando arcos donde ND es destino
    list<pairCC>::iterator j;
    for (j = adjb[node_id].begin(); j != adjb[node_id].end(); ++j)
    {
        if(Nodes[node_id].get_type() == 1)      //caso que nodo analizado sea PLanta, solo me interesan productos consolidados, no supermercado
        {
            *primario = false;
            string NO = Nodes[Edges[j->edge].v()].get_cod();
           
            set<string>::iterator it = Plantas.find(NO);
            if(it == Plantas.end())        //no se encontró planta, esto no debería ocurrir
            {
                cerr << "No se encontró planta, error en IdentificaOrigen" << endl;
                exit(1);
            }
            long pos_id = distance(Plantas.begin(),it);     //posición en Plantas de planta origen
            for(unsigned int i=0; i< Edges[j->edge].CamionesD.size(); i++)
            {
                for(unsigned int m=0; m<Edges[j->edge].CamionesD[i].carga.size(); m++)
                {
                    for(unsigned int p=0; p<Edges[j->edge].CamionesD[i].carga[m].BC.size(); p++)
                    {
                        if(Edges[j->edge].CamionesD[i].carga[m].BC[p].getProduct(Prod) == CodProducto)
                        {
                            Ncajas[0][pos_id] += Edges[j->edge].CamionesD[i].carga[m].BC[p].getCajas();
                        }
                    }
                }
            }
            
        }       // end if caso que sea planta-planta
 
         else if(Nodes[node_id].get_type() == 2)     //nodo destino, por lo que miro supermercado y no supermercado
        {
            *primario = true;
            string NO = Nodes[Edges[j->edge].v()].get_cod();
            
            set<string>::iterator it = Plantas.find(NO);
            if(it == Plantas.end())        //si el nodo origen es sucursal, estamos en caso Sucursal-Sucursal, es decir el segundo arco de P-S-S
            {
                for(unsigned int i=0; i< Edges[j->edge].CamionesS.size(); i++)
                {
                    for(unsigned int w=0; w<Edges[j->edge].CamionesS[i].size(); w++)
                    {
                        for(unsigned int m=0; m<Edges[j->edge].CamionesS[i][w].carga.size(); m++)
                        {
                            //identificando planta de origen de este pallet
                            int posPE = Edges[j->edge].CamionesS[i][w].carga[m].get_prev_edge();
                            string NOP;
                            if(posPE == -1)
                                NOP = Nodes[Edges[j->edge].v()].get_cod();
                            else
                                NOP = Nodes[Edges[posPE].v()].get_cod();
                            
                            set<string>::iterator it2 = Plantas.find(NOP);
                            if(it2 == Plantas.end())        //no se encontró planta, esto no debería ocurrir
                            {
                                cerr << "No se encontró planta, error en IdentificaOrigen" << endl;
                                exit(1);
                            }
                            
                            long pos_idP = distance(Plantas.begin(),it2);     //posición en Plantas de planta origen
                            for(unsigned int p=0; p<Edges[j->edge].CamionesS[i][w].carga[m].BC.size(); p++)
                            {
                                if(Edges[j->edge].CamionesS[i][w].carga[m].BC[p].getProduct(Prod) == CodProducto)
                                {
                                    
                                    if(Edges[j->edge].CamionesS[i][w].carga[m].BC[p].getSuper())
                                    {
                                        Ncajas[1][pos_idP] += Edges[j->edge].CamionesS[i][w].carga[m].BC[p].getCajas(); //caso supermercado
                                    }
                                    else
                                    {
                                        Ncajas[0][pos_idP] += Edges[j->edge].CamionesS[i][w].carga[m].BC[p].getCajas(); //no supermercado
                                    }
                                }
                            }
                        }
                    }
                }       // end for camionesS
                
            }       // en if caso CamionesS en arco S-S
            else    // existe planta origen, estamos en arco P-S
            {
                long pos_id = distance(Plantas.begin(),it);     //posición en Plantas de planta origen
                for(unsigned int i=0; i< Edges[j->edge].CamionesD.size(); i++)
                {
                    for(unsigned int m=0; m<Edges[j->edge].CamionesD[i].carga.size(); m++)
                    {
                        for(unsigned int p=0; p<Edges[j->edge].CamionesD[i].carga[m].BC.size(); p++)
                        {
                            if(Edges[j->edge].CamionesD[i].carga[m].BC[p].getProduct(Prod) == CodProducto)
                            {
                                
                                if(Edges[j->edge].CamionesD[i].carga[m].BC[p].getSuper())
                                {
                                    Ncajas[1][pos_id] += Edges[j->edge].CamionesD[i].carga[m].BC[p].getCajas(); //caso supermercado
                                }
                                else
                                {
                                    Ncajas[0][pos_id] += Edges[j->edge].CamionesD[i].carga[m].BC[p].getCajas(); //no supermercado
                                }
                            }
                        }
                    }
                }       // end for para camiones D
                
                // camiones2 arco P-S
                for(unsigned int i=0; i< Edges[j->edge].Camiones2.size(); i++)
                {
                    for(unsigned int w=0; w<Edges[j->edge].Camiones2[i].size(); w++)
                    {
                        for(unsigned int m=0; m<Edges[j->edge].Camiones2[i][w].carga.size(); m++)
                        {
                            //identificando planta de origen de este pallet, si no existe prev edge es un pallet que se cargó en el origen de este nodo
                            int posPE = Edges[j->edge].Camiones2[i][w].carga[m].get_prev_edge();
                            string NOP;
                            if(posPE == -1)
                                NOP = Nodes[Edges[j->edge].v()].get_cod();
                            else
                                NOP = Nodes[Edges[posPE].v()].get_cod();
                            
                            set<string>::iterator it2 = Plantas.find(NOP);
                            if(it2 == Plantas.end())        //no se encontró planta, esto no debería ocurrir
                            {
                                cerr << "No se encontró planta, error en IdentificaOrigen" << endl;
                                exit(1);
                            }
                            
                            long pos_idP = distance(Plantas.begin(),it2);     //posición en Plantas de planta origen
                            for(unsigned int p=0; p<Edges[j->edge].Camiones2[i][w].carga[m].BC.size(); p++)
                            {
                                if(Edges[j->edge].Camiones2[i][w].carga[m].BC[p].getProduct(Prod) == CodProducto)
                                {
            
                                    if(Edges[j->edge].Camiones2[i][w].carga[m].BC[p].getSuper())
                                    {
                                        Ncajas[1][pos_idP] += Edges[j->edge].Camiones2[i][w].carga[m].BC[p].getCajas(); //caso supermercado
                                    }
                                    else
                                    {
                                        Ncajas[0][pos_idP] += Edges[j->edge].Camiones2[i][w].carga[m].BC[p].getCajas(); //no supermercado
                                    }
                                }
                            }
                        }
                    }
                }       // end for camiones2
                
                if(Edges[j->edge].CamionesS.size()>0)
                {
                    for(unsigned int i=0; i< Edges[j->edge].CamionesS.size(); i++)
                    {
                        for(unsigned int w=0; w<Edges[j->edge].CamionesS[i].size(); w++)
                        {
                            for(unsigned int m=0; m<Edges[j->edge].CamionesS[i][w].carga.size(); m++)
                            {
                                for(unsigned int p=0; p<Edges[j->edge].CamionesS[i][w].carga[m].BC.size(); p++)
                                {
                                    if((Edges[j->edge].CamionesS[i][w].carga[m].BC[p].getProduct(Prod) == CodProducto)&&(Edges[j->edge].CamionesS[i][w].carga[m].get_descarga() == 1))
                                    {
                                        if(Edges[j->edge].CamionesS[i][w].carga[m].BC[p].getSuper())
                                            Ncajas[1][pos_id] += Edges[j->edge].CamionesS[i][w].carga[m].BC[p].getCajas(); //caso supermercado
                                        else
                                            Ncajas[0][pos_id] += Edges[j->edge].CamionesS[i][w].carga[m].BC[p].getCajas(); //no supermercado
                                    }
                                }
                            }
                        }
                    }       // end for camionesS
                    
                }   // end if caso CamionesS en arco P-S
                
            }       // end else

        }       // end else-if nodos tipo 2
    
    }       // end for identificando arcos accediendo
    
    return(Ncajas);
    
}   // end fn

// imprime los labels usando la estructura Edges
string Graph::PrintRutas(vector<Edge>& Edges, vector<Node>& Nodes, vector<Producto>& Prod, vector<string>& sectores)
{
    stringstream s;
    vector<RutaPrint> Resultados;
 //   s << "Rutas asignación " << endl;
    
    vector<string> estados = {"Refrigerado","Congelado"};
    size_t Nsectores = sectores.size();
    size_t Nestados = estados.size();

    for(unsigned int i=0; i<Edges.size(); i++)
    {
        if((Nodes[Edges[i].v()].get_type() == 1) && (Nodes[Edges[i].w()].get_type() == 1))
        {
            if(Edges[i].CamionesD.size()>0)
            {
                for(unsigned int j=0; j<Edges[i].CamionesD.size(); j++)
                {
                    string sec0 = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                    Resultados.push_back(RutaPrint(sec0,Edges[i].CamionesD[j].get_stype(),Edges[i].CamionesD[j].get_idc(),Nsectores,Nestados, Edges[i].CamionesD[j].ClasifCamion));
                    Resultados.back().planta1 = Nodes[Edges[i].v()].get_cod();
                    Resultados.back().costo0 = Edges[i].getCostT(Edges[i].CamionesD[j].get_stype());
                    
                    if(Edges[i].CamionesD[j].carga.size()>0)
                    {
                        for(unsigned int k=0; k<Edges[i].CamionesD[j].carga.size(); k++)
                        {
                            if(Edges[i].CamionesD[j].carga[k].BC.size()>0)
                            {
                                for(unsigned int m=0; m<Edges[i].CamionesD[j].carga[k].BC.size(); m++)
                                {
                                    for(unsigned z=0; z<sectores.size(); z++)
                                    {
                                        if(Edges[i].CamionesD[j].carga[k].BC[m].getEstado(Prod) == estados[0])
                                        {
                                            if((Edges[i].CamionesD[j].carga[k].BC[m].getSector(Prod) == sectores[z]))
                                                Resultados.back().carga1[0][z] += Edges[i].CamionesD[j].carga[k].BC[m].ocupBunch(Prod);
                                        }
                                        else if(Edges[i].CamionesD[j].carga[k].BC[m].getEstado(Prod) == estados[1])
                                        {
                                            if((Edges[i].CamionesD[j].carga[k].BC[m].getSector(Prod) == sectores[z]))
                                                Resultados.back().carga1[1][z] += Edges[i].CamionesD[j].carga[k].BC[m].ocupBunch(Prod);
                                        }
                                    }       // end for z
                    
                                }   // end for m
                            }
                                
                        }       // end for k
                    }
                    
                }           // end for j
                            
            }       // end if CamionesD
            
            // caso de Camiones2 (planta a planta) ++++++++++++++++++++++++++++++++++
            if(Edges[i].Camiones2.size() > 0)
            {
                for(unsigned int j=0; j<Edges[i].Camiones2.size(); j++)
                {
                    string sec0 = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                    
                    if(Edges[i].Camiones2[j].size()>0)
                    {
                        for(unsigned int l=0; l<Edges[i].Camiones2[j].size(); l++)
                        {
                            Resultados.push_back(RutaPrint(sec0,Edges[i].Camiones2[j][l].get_stype(),Edges[i].Camiones2[j][l].get_idc(),Nsectores,Nestados, Edges[i].Camiones2[j][l].ClasifCamion));
                            Resultados.back().planta1 = Nodes[Edges[i].v()].get_cod();
                            
                            Resultados.back().costo0 = Edges[i].getCostT(Edges[i].Camiones2[j][l].get_stype());
                            
                            if(Edges[i].Camiones2[j][l].carga.size()>0)
                            {
                                for(unsigned int k=0; k<Edges[i].Camiones2[j][l].carga.size(); k++)
                                {
                                    if(Edges[i].Camiones2[j][l].carga[k].BC.size()>0)
                                    {
                                        for(unsigned int m=0; m<Edges[i].Camiones2[j][l].carga[k].BC.size(); m++)
                                        {
                                            for(unsigned z=0; z<sectores.size(); z++)
                                            {
                                                if(Edges[i].Camiones2[j][l].carga[k].BC[m].getEstado(Prod) == estados[0])
                                                {
                                                    if((Edges[i].Camiones2[j][l].carga[k].BC[m].getSector(Prod) == sectores[z]))
                                                        Resultados.back().carga1[0][z] += Edges[i].Camiones2[j][l].carga[k].BC[m].ocupBunch(Prod);
                                                }
                                                else if(Edges[i].Camiones2[j][l].carga[k].BC[m].getEstado(Prod) == estados[1])
                                                {
                                                    if((Edges[i].Camiones2[j][l].carga[k].BC[m].getSector(Prod) == sectores[z]))
                                                        Resultados.back().carga1[1][z] += Edges[i].Camiones2[j][l].carga[k].BC[m].ocupBunch(Prod);
                                                }
                                            }   // end for z
                                    
                                        }   // end for m
                                    }
                                    
                                }       // end for k
                            }
                            
                        }       // end for l
                    }
                    
                }           // end for j
            
            }   // end if para Camiones2 en arco Planta a planta
            
        }   // end if planta-planta
        
    }             // end for i recorriendo todos los arcos
  
    for(unsigned int i=0; i<Edges.size(); i++)
    {
        if((Nodes[Edges[i].v()].get_type() == 1) && (Nodes[Edges[i].w()].get_type() == 2))
        {
            if(Edges[i].CamionesD.size()>0)
            {
                for(unsigned int j=0; j<Edges[i].CamionesD.size(); j++)
                {
                    string sec0 = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                    Resultados.push_back(RutaPrint(sec0,Edges[i].CamionesD[j].get_stype(),Edges[i].CamionesD[j].get_idc(),Nsectores,Nestados, Edges[i].CamionesD[j].ClasifCamion));
                    Resultados.back().planta1 = Nodes[Edges[i].v()].get_cod();
                    
                    Resultados.back().costo0 = Edges[i].getCostT(Edges[i].CamionesD[j].get_stype());
                    
                    if(Edges[i].CamionesD[j].carga.size()>0)
                    {
                        for(unsigned int k=0; k<Edges[i].CamionesD[j].carga.size(); k++)
                        {
                            if(Edges[i].CamionesD[j].carga[k].BC.size()>0)
                            {
                                for(unsigned int m=0; m<Edges[i].CamionesD[j].carga[k].BC.size(); m++)
                                {
                                    for(unsigned z=0; z<sectores.size(); z++)
                                    {
                                        if(Edges[i].CamionesD[j].carga[k].BC[m].getEstado(Prod) == estados[0])
                                        {
                                            if((Edges[i].CamionesD[j].carga[k].BC[m].getSector(Prod) == sectores[z]))
                                                Resultados.back().carga1[0][z] += Edges[i].CamionesD[j].carga[k].BC[m].ocupBunch(Prod);
                                        }
                                        else if(Edges[i].CamionesD[j].carga[k].BC[m].getEstado(Prod) == estados[1])
                                        {
                                            if((Edges[i].CamionesD[j].carga[k].BC[m].getSector(Prod) == sectores[z]))
                                                Resultados.back().carga1[1][z] += Edges[i].CamionesD[j].carga[k].BC[m].ocupBunch(Prod);
                                        }
                                    }
                    
                                }   // end for m
                            }
                            
                        }       // end for k
                    }
                    
                }           // end for j
                            
            }       // end if CamionesD
            
            //caso CamionesS primer tramo, en estos camiones todo se carga en planta única, que queda en planta 1
            if(Edges[i].CamionesS.size() > 0)
            {
                for(unsigned int j=0; j<Edges[i].CamionesS.size(); j++)
                {
                    string sec0 = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                    
                    if(Edges[i].CamionesS[j].size()>0)
                    {
                        for(unsigned int l=0; l<Edges[i].CamionesS[j].size(); l++)
                        {
                            Resultados.push_back(RutaPrint(sec0,Edges[i].CamionesS[j][l].get_stype(),Edges[i].CamionesS[j][l].get_idc(),Nsectores,Nestados, Edges[i].CamionesS[j][l].ClasifCamion));
                            Resultados.back().planta1 = Nodes[Edges[i].v()].get_cod();      //acá se carga todo el producto
                            Resultados.back().costo0 = Edges[i].getCostT(Edges[i].CamionesS[j][l].get_stype());
                            
                            if(Edges[i].CamionesS[j][l].carga.size()>0)
                            {
                                for(unsigned int k=0; k<Edges[i].CamionesS[j][l].carga.size(); k++)
                                {
                                    if(Edges[i].CamionesS[j][l].carga[k].BC.size()>0)
                                    {
                                        for(unsigned int m=0; m<Edges[i].CamionesS[j][l].carga[k].BC.size(); m++)
                                        {
                                            for(unsigned z=0; z<sectores.size(); z++)
                                            {
                                                if(Edges[i].CamionesS[j][l].carga[k].BC[m].getEstado(Prod) == estados[0])
                                                {
                                                    if((Edges[i].CamionesS[j][l].carga[k].BC[m].getSector(Prod) == sectores[z]))
                                                        Resultados.back().carga1[0][z] += Edges[i].CamionesS[j][l].carga[k].BC[m].ocupBunch(Prod);
                                                }
                                                else if(Edges[i].CamionesS[j][l].carga[k].BC[m].getEstado(Prod) == estados[1])
                                                {
                                                    if((Edges[i].CamionesS[j][l].carga[k].BC[m].getSector(Prod) == sectores[z]))
                                                        Resultados.back().carga1[1][z] += Edges[i].CamionesS[j][l].carga[k].BC[m].ocupBunch(Prod);
                                                }
                                            }   // end for z
                                    
                                        }   // end for m
                                    }
                                }       // end for k
                            }
                        }       // end for l
                    }
                    
                }           // end for j
                
            }       // enc caso CamionesS, tramo P-S
            
            // caso de Camiones2 (planta a sucursal) ++++++++++++++++++++++++++++++++++
            if(Edges[i].Camiones2.size() > 0)
            {
                for(unsigned int j=0; j<Edges[i].Camiones2.size(); j++)
                {
                    string sec0 = "-" + Nodes[Edges[i].w()].get_cod();
                    if(Edges[i].Camiones2[j].size()>0)
                    {
                        for(unsigned int l=0; l<Edges[i].Camiones2[j].size(); l++)
                        {
                            int ii,jj;
                            int id_camion = Edges[i].Camiones2[j][l].get_idc();
                            int prev_edge = Edges[i].Camiones2[j][l].get_prev();
                            Edges[prev_edge].searchCamion(id_camion, &ii, &jj);
                            bool cond = true;
                            int theta = 0;
                            while(cond)
                            {
                                if(Resultados[theta].camion_id == id_camion)
                                    cond = false;
                                theta++;
                            }
                            int idPP = theta-1;
                            Resultados[idPP].actualiza_secuencia(sec0);
                            Resultados[idPP].planta2 = Nodes[Edges[i].v()].get_cod();
                            
                            /*if(Edges[i].Camiones2[j][l].get_stype() == "Truck1")
                                Resultados[idPP].costo1 = Edges[i].getCostT1();
                            else if(Edges[i].Camiones2[j][l].get_stype() == "Truck2")
                                Resultados[idPP].costo1 = Edges[i].getCostT2();*/
                            Resultados[idPP].costo1 = Edges[i].getCostT(Edges[i].Camiones2[j][l].get_stype());
                            
                            int iii,jjj,kkk;
                            bool rr;
                            
                            if(Edges[i].Camiones2[j][l].carga.size()>0)
                            {
                                for(unsigned int k=0; k<Edges[i].Camiones2[j][l].carga.size(); k++)
                                {
                                    bool condPP = Edges[prev_edge].searchPallet(Edges[i].Camiones2[j][l].carga[k].getid_pallet(), &rr, &iii, &jjj, &kkk);
                                    if(!condPP)
                                    {
                                        if(Edges[i].Camiones2[j][l].carga[k].BC.size()>0)
                                        {
                                            for(unsigned int m=0; m<Edges[i].Camiones2[j][l].carga[k].BC.size(); m++)
                                            {
                                                for(unsigned z=0; z<sectores.size(); z++)
                                                {
                                                    if(Edges[i].Camiones2[j][l].carga[k].BC[m].getEstado(Prod) == estados[0])
                                                    {
                                                        if((Edges[i].Camiones2[j][l].carga[k].BC[m].getSector(Prod) == sectores[z]))
                                                            Resultados[idPP].carga2[0][z] += Edges[i].Camiones2[j][l].carga[k].BC[m].ocupBunch(Prod);
                                                    }
                                                    else if(Edges[i].Camiones2[j][l].carga[k].BC[m].getEstado(Prod) == estados[1])
                                                    {
                                                        if((Edges[i].Camiones2[j][l].carga[k].BC[m].getSector(Prod) == sectores[z]))
                                                            Resultados[idPP].carga2[1][z] += Edges[i].Camiones2[j][l].carga[k].BC[m].ocupBunch(Prod);
                                                    }
                                                }   // end for z
                                                                          
                                            }   // end for m
                                        }
                                        
                                    }       // end cond se carga sólo si no venía antes
                                    
                                }       // end for k
                            }

                        }       // end for l
                    }
                }           // end for j
                    
                    
            }       // end if Camiones2 size > 0
   
        }   // end if para Camiones2 en arco Planta a sucursal
            
    }       // end for i recorriendo todos los arcos
    
    for(unsigned int i=0; i<Edges.size(); i++)
    {
        if((Nodes[Edges[i].v()].get_type() == 2) && (Nodes[Edges[i].w()].get_type() == 2))  //acá se completan las rutas asociadas a viajes combinados. Toda la carga ya se registró en primer tramo
        {
            // caso de CamionesS (sucursal a sucursal) ++++++++++++++++++++++++++++++++++
            if(Edges[i].CamionesS.size() > 0)
            {
                for(unsigned int j=0; j<Edges[i].CamionesS.size(); j++)
                {
                    string sec0 = "-" + Nodes[Edges[i].w()].get_cod();
                    if(Edges[i].CamionesS[j].size()>0)
                    {
                        for(unsigned int l=0; l<Edges[i].CamionesS[j].size(); l++)
                        {
                            int ii,jj;
                            int id_camion = Edges[i].CamionesS[j][l].get_idc();
                            int prev_edge = Edges[i].CamionesS[j][l].get_prev();
                            Edges[prev_edge].searchCamionS(id_camion, &ii, &jj);
                            bool cond = true;
                            int theta = 0;
                            while(cond)
                            {
                                if(Resultados[theta].camion_id == id_camion)
                                    cond = false;
                                theta++;
                            }
                            int idPP = theta-1;
                            Resultados[idPP].actualiza_secuencia(sec0);
       //                   Resultados[idPP].planta2 = Nodes[Edges[i].v()].get_cod();
                            Resultados[idPP].costo1 = Edges[i].getCostT(Edges[i].CamionesS[j][l].get_stype());

                        }       // end for l
                    }           // end if

                }           // end for j
            }               // end if CamionesS size

        }   // end if arcos S-S
    }       // end for i recorriendo todos los arcos
    
    // Ahora se imprimen los resultados
    s << setiosflags(ios_base::fixed);
    for(unsigned int i=0; i<Resultados.size(); i++)
    {
        int sep = Resultados[i].separador();
  //      cout << "i: " << i << "  Res size: " << Resultados.size() << "  Check1: " << Resultados[i].check_carga(1) << "  Check2: " << Resultados[i].check_carga(2) << endl;
        if(Resultados[i].check_carga(1))
        {
            string FEEDC = "";      //se crea string vacío
            int edge_id = Resultados[i].Find_Edge(Edges,Nodes,Resultados[i].planta1);
            if(edge_id == -1)
            {
                cerr << "error, edge id no identificado con carga positiva" << endl;
                exit(-1);
            }

    //        cout << "Sec1: " << Resultados[i].Secuencia << " P1: " << Resultados[i].planta1 << "  edge: " << edge_id << endl;
            
            vector<int> CamionesID;
            int m,n;
            int CamFeed = Edges[edge_id].searchCamionTotal(Resultados[i].camion_id, &m, &n);
            bool cond = false;
            if(CamFeed == 1)    //encontró camión en CamionesD
                cond = Find_camion_feeder_interplanta(CamionesID, Edges[edge_id].CamionesD[m], Edges, Nodes);
            else if(CamFeed == 2)
                cond = Find_camion_feeder_interplanta(CamionesID, Edges[edge_id].Camiones2[m][n], Edges, Nodes);
            else if(CamFeed == 3)
                cond = Find_camion_feeder_interplanta(CamionesID, Edges[edge_id].CamionesS[m][n], Edges, Nodes);
            
            
            if(cond)
            {
                FEEDC.append(Resultados[i].planta1);
                FEEDC += "(";
                if(CamionesID.size()>1)
                {
                    for(unsigned kk=0; kk<(CamionesID.size()-1); kk++)
                        FEEDC += to_string(CamionesID[kk]) + "-";
                }
                FEEDC += to_string(CamionesID[CamionesID.size()-1]) + ")";
            }
                
            for(unsigned int j=0; j<Resultados[i].carga1.size(); j++)
            {
                for(unsigned int k=0; k<Resultados[i].carga1[j].size(); k++)
                {
                    if(Resultados[i].carga1[j][k] > 0)
                        s << Resultados[i].Secuencia << ";" << Resultados[i].planta1 << ";" << Resultados[i].tipo_camion << ";" << Resultados[i].camion_id << ";" << sectores[k] << ";" << estados[j] << ";" << setprecision(NDEC) << Resultados[i].carga1[j][k] << ";" << setprecision(0) << sep << ";" << Resultados[i].costo0 + Resultados[i].costo1 << ";" << Resultados[i].ClasifCamion << ";" << FEEDC << ";" << endl;
                }   // end for
                    
            }       // end for carga1
        }
        if(Resultados[i].check_carga(2))
        {
            string FEEDC = "";      //se crea string vacío
            int edge_id = Resultados[i].Find_Edge(Edges,Nodes,Resultados[i].planta2);
            if(edge_id == -1)
            {
                cerr << "error, edge id no identificado con carga positiva" << endl;
                exit(-1);
            }
            
  //          cout << "Sec2: " << Resultados[i].Secuencia << " P2: " << Resultados[i].planta2 << "  edge: " << edge_id << endl;
            
            vector<int> CamionesID;
            int m,n;
            int CamFeed = Edges[edge_id].searchCamionTotal(Resultados[i].camion_id, &m, &n);
            bool cond = false;
            if(CamFeed == 1)    //encontró camión en CamionesD
                cond = Find_camion_feeder_interplanta(CamionesID, Edges[edge_id].CamionesD[m], Edges, Nodes);
            else if(CamFeed == 2)
                cond = Find_camion_feeder_interplanta(CamionesID, Edges[edge_id].Camiones2[m][n], Edges, Nodes);
            else if(CamFeed == 3)
                cond = Find_camion_feeder_interplanta(CamionesID, Edges[edge_id].CamionesS[m][n], Edges, Nodes);
            if(cond)
            {
                FEEDC.append(Resultados[i].planta1);
                FEEDC += "(";
                if(CamionesID.size()>1)
                {
                    for(unsigned kk=0; kk<(CamionesID.size()-1); kk++)
                        FEEDC += to_string(CamionesID[kk]) + "-";
                }
                FEEDC += to_string(CamionesID[CamionesID.size()-1]) + ")";
            }
            
            for(unsigned int j=0; j<Resultados[i].carga2.size(); j++)
            {
                for(unsigned int k=0; k<Resultados[i].carga2[j].size(); k++)
                {
                    if(Resultados[i].carga2[j][k] > 0)
                        s << Resultados[i].Secuencia << ";" << Resultados[i].planta2 << ";" << Resultados[i].tipo_camion << ";" << Resultados[i].camion_id << ";" << sectores[k] << ";" << estados[j] << ";" << setprecision(NDEC) << Resultados[i].carga2[j][k] << ";" << setprecision(0) << sep << ";" << Resultados[i].costo0 + Resultados[i].costo1 << ";" << Resultados[i].ClasifCamion << ";" << FEEDC << ";" << endl;
                }
                    
            }       // end carga2
        }
       
    }
    
    return s.str();
}

//cuenta el número de cajas generadas
string Graph::PrintCuadratura(vector<Edge>& Edges, vector<Node>& Nodes, vector<Producto>& Prod)
{
    stringstream s;
    s << setiosflags(ios_base::fixed);
    
    for(unsigned int k = 0; k < V; k++)
    {
        string code_destino = Nodes[k].get_cod();
        for(unsigned int m=0; m<Prod.size(); m++)
        {
            list<pairCC>::iterator j;
            double sumP = 0;
            for(j = adjb[k].begin(); j != adjb[k].end(); ++j)
            {
                sumP += Edges[j->edge].Cajas_producto(Prod[m].get_code(), Nodes, Prod);
            }
            if(sumP > 0)
                s << code_destino << ";" << Prod[m].get_code() << ";" << setprecision(NDEC) << sumP << ";" << endl;
        }       // end for m
        
    }   // end for k
    return s.str();
}
 
 
#endif



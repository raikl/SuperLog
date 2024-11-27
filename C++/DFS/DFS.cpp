// Reporting.cpp: define el punto de entrada de la aplicación de consola.
//

#include "CargaDatos.h"

#if defined(_MSC_VER)
#define sprintf sprintf_s
#endif

// crea un set con las plantas
set<string> CreaSetPlantas(vector <Node>& Nodes)
{
    set<string> ss;
    for(unsigned int k=0; k<Nodes.size(); k++)
    {
//        cout << "Nodo: " << Nodes[k].get_cod() << "  type: " << Nodes[k].get_type() << endl;
        if(Nodes[k].get_type() == 1)
            ss.insert(Nodes[k].get_cod());
    }
    return(ss);
}

//************************** FUNCIONES PARA CARGAR CAMIONES, PRIMERO PRODUCTOS A PALLETS Y DESPUÉS PALLETS A CAMIONES ************************

void Cargando_pallets(Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, vector<Producto>& Prod, vector<Edge_parts>& SOB, vector<Pallet>& SOBP, vector<Familia>& FA)
{
	int Cam = 0;
    vector<vector<Pallet>> CargaAux;
    bool cond = false;
    int PalletN = 1;
	for (unsigned int i = 0; i<Edges.size(); i++)
	{
        if((Edges[i].y.size()>0) || (cond))
        {
            vector<Camion> caux;
            Edges[i].cargaPalletsProductos(PalletN, Prod, Edges[i].x, Edges[i].z, Edges[i].CargaD,-1,-1,-1, SOB,FA);
            Edges[i].LoadCamionesEdge(Cam,caux,Edges[i].CamionesD, Edges[i].CargaD, CargaAux, Edges[i].y, Prod, Nodes, -1,-1,SOBP, false,FA);
            Cam += Edges[i].CamionesD.size();
        }
	}       // end for

}

//reduciendo pallets. Output son los pallets reducidos desde pp. En BCS quedan las cajas que se bajan
vector<Pallet> Reduciendo_pallets(int edge_id, double peso_a_reducir, vector<Pallet>& pp, vector<Bunch_cajas>& BCS, vector<Producto>& Prod)
{
  //  se trata de empezar a reducir a ls que tengan mayor cuadratura primero
    for(unsigned int k=0; k<pp.size(); k++)
        pp[k].OcupPesoCuad(Prod);
    sort(pp.begin(), pp.end(), &CompareCUAD);
    double faltante = peso_a_reducir;

    vector<Pallet> aux;
    aux.clear();
    BCS.clear();
    
    for(unsigned int k=0; k<pp.size(); k++)
    {
        pp[k].OcupPesoCuad(Prod);
        double UmbralPeso = F3*pp[k].peso;
        double targetR = pp[k].peso - UmbralPeso;
        if(faltante < UmbralPeso)
            targetR = pp[k].peso - faltante;

        pp[k].desarma_pallet2(targetR, BCS, Prod);
        aux.push_back(pp[k]);
        double reduccion = min(UmbralPeso,faltante);
        faltante -= reduccion;
        
        if(faltante <= 0)
            break;
    }
    return(aux);
}                   // end reduciendoPallets

void FindModifyPallet(int current_edge_id, Pallet PMod, vector<Edge>& Edges, vector<Producto>& Prod)
{
    for(unsigned pp=0; pp<Edges.size(); pp++)
    {
        for(unsigned int jj=0; jj<Edges[pp].CamionesD.size(); jj++)
        {
            for(unsigned int kk=0; kk<Edges[pp].CamionesD[jj].carga.size(); kk++)
            {
                if((Edges[pp].CamionesD[jj].carga[kk].getid_pallet() == PMod.getid_pallet())&&(current_edge_id != pp))
                {
                    Edges[pp].CamionesD[jj].carga[kk].BC = PMod.BC;
                    Edges[pp].CamionesD[jj].carga[kk].OcupPesoCuad(Prod);
                }
            }
        }
        
        for(unsigned int ii=0; ii<Edges[pp].Camiones2.size(); ii++)
        {
            for(unsigned int jj=0; jj<Edges[pp].Camiones2[ii].size(); jj++)
            {
                for(unsigned int kk=0; kk<Edges[pp].Camiones2[ii][jj].carga.size(); kk++)
                {
                    if((Edges[pp].Camiones2[ii][jj].carga[kk].getid_pallet() == PMod.getid_pallet())&&(current_edge_id != pp))
                    {
                        Edges[pp].Camiones2[ii][jj].carga[kk].BC = PMod.BC;
                        Edges[pp].Camiones2[ii][jj].carga[kk].OcupPesoCuad(Prod);
                    }
                }
            }
        }       // end for Camiones2
        
        for(unsigned int ii=0; ii<Edges[pp].CamionesS.size(); ii++)
        {
            for(unsigned int jj=0; jj<Edges[pp].CamionesS[ii].size(); jj++)
            {
                for(unsigned int kk=0; kk<Edges[pp].CamionesS[ii][jj].carga.size(); kk++)
                {
                    if((Edges[pp].CamionesS[ii][jj].carga[kk].getid_pallet() == PMod.getid_pallet())&&(current_edge_id != pp))
                    {
                        Edges[pp].CamionesS[ii][jj].carga[kk].BC = PMod.BC;
                        Edges[pp].CamionesS[ii][jj].carga[kk].OcupPesoCuad(Prod);
                    }
                }
            }
        }       // end for CamionesS

    }

}       //end función que modifica pallet

//si está en SOB, return1, si está en CamAux, return 2, si está en CamC, return 3
int FindPalletCamiones(int pallet_id, vector<Pallet> SOB, vector<Camion>& CamAux, vector<Camion>& CamC, int* pos1, int* pos2)
{
    for(unsigned m=0; m<SOB.size(); m++)
    {
        if(SOB[m].getid_pallet() == pallet_id)
        {
            *pos1 = m;
            *pos2 = -1;
            return(1);
        }
    }
    for(unsigned int i=0; i<CamAux.size(); i++)
    {
        for(unsigned int j=0; j<CamAux[i].carga.size(); j++)
        {
            if(CamAux[i].carga[j].getid_pallet() == pallet_id)
            {
                *pos1 = i;
                *pos2 = j;
                return(2);
            }
        }
    }
    for(unsigned int i=0; i<CamC.size(); i++)
    {
        for(unsigned int j=0; j<CamC[i].carga.size(); j++)
        {
            if(CamC[i].carga[j].getid_pallet() == pallet_id)
            {
                *pos1 = i;
                *pos2 = j;
                return(3);
            }
        }
    }
    return(-1);
}   //end FindPalletCamiones

void EjecutaReduccion(int edge_id, vector<Camion>& CamAux, vector<Camion>& CCons, vector<Pallet>& SOBPA3, vector<Pallet>& PalletsReducidos, vector<Bunch_cajas>& BCS, vector<Producto>& Prod, vector<Familia>& FA)
{
    double peso_total_abajo = 0;
    double peso_a_reducir = 0;
    bool cong = FA[SOBPA3[0].familia].congelado;
    for(unsigned int k=0; k<SOBPA3.size(); k++)
    {
        SOBPA3[k].OcupPesoCuad(Prod);
        peso_total_abajo += SOBPA3[k].peso;
    }
    
    if(SOBPA3.size() == 1)
        peso_a_reducir = F1*peso_total_abajo;
    else if(SOBPA3.size() > 1)
        peso_a_reducir = F2*peso_total_abajo;
    
    vector<Pallet> Aux;
    Aux.clear();
    for(unsigned m=0; m<SOBPA3.size(); m++)
        Aux.push_back(SOBPA3[m]);
    for(unsigned int ii=0; ii<CamAux.size();ii++)
    {
        for(unsigned int j=0; j<CamAux[ii].carga.size(); j++)
        {
            if(cong == FA[CamAux[ii].carga[j].familia].congelado)
                Aux.push_back(CamAux[ii].carga[j]);
        }
    }
    if(CCons.size()>0)
    {
        for(unsigned int ii=0; ii<CCons.size();ii++)
        {
            for(unsigned int j=0; j<CCons[ii].carga.size(); j++)
            {
                if(cong == FA[CCons[ii].carga[j].familia].congelado)
                    Aux.push_back(CCons[ii].carga[j]);
            }
        }
    }

    PalletsReducidos = Reduciendo_pallets(edge_id, peso_a_reducir, Aux, BCS, Prod);
    
    for(unsigned int m=0; m<PalletsReducidos.size(); m++)
    {
        int pos1, pos2;
        int tipoM = FindPalletCamiones(PalletsReducidos[m].getid_pallet(), SOBPA3, CamAux, CCons, &pos1, &pos2);
        if(tipoM == 1)
            SOBPA3[pos1] = PalletsReducidos[m];
        else if(tipoM == 2)
            CamAux[pos1].carga[pos2] = PalletsReducidos[m];
        else if(tipoM == 3)
            CCons[pos1].carga[pos2] = PalletsReducidos[m];
    }
}       //fin de reducción de pallets

void IterandoCargaSobrante(int edge_id, bool PP, vector<Camion>& CamAux, vector<Camion>& CCons, vector<Edge>& Edges, vector<Pallet>& SOBPI, vector<Pallet>& SOBPF, vector<Producto>& Prod, vector<Familia>& FA)
{
    
    for(unsigned int p=0; p<NITERSP; p++)
    {
        if(PP)
            Edges[edge_id].FixPalletsNoCargadosPP(CamAux, SOBPI, NOPTIONS, Prod, SOBPF, FA);
        else
            Edges[edge_id].FixPalletsNoCargadosPS(CamAux, CCons, SOBPI, NOPTIONS, Prod, SOBPF, FA);
                
        if(SOBPF.size() == 0)     // logré cargar el pallet correctamente
        {
            SOBPI.clear();
            break;
        }
        else
        {
            SOBPI.clear();
            for(unsigned int p=0; p<SOBPF.size(); p++)
                SOBPI.push_back(SOBPF[p]);
    
            SOBPF.clear();
        }       // end else
        
    }   // end for
    SOBPF = SOBPI;
    
}   // end function


// cargando sobrante, que no pude cargar en arcos directos (consolidación, es decir planta-planta, i es el id del edge
// sólo trata de cargar el primer elemento de sobrante SOBPA, si aparecen más los trata de cargar en una nueva llamada a la función
void Cargando_sobranteIND_PP(int i, vector<Camion>& CamAux, vector<Edge>& Edges, vector<Pallet>& SOBPA, vector<Pallet>& SOBPA3, vector<Producto>& Prod, vector<Familia>& FA)
{
    vector<Pallet> SOBPA2;
    SOBPA2.clear();
    SOBPA2 = SOBPA;
//    SOBPA2.push_back(SOBPA[idSOB]);
    
    vector<Camion> CamAux2;
    CamAux2.clear();
    IterandoCargaSobrante(i, true, CamAux, CamAux2, Edges, SOBPA2, SOBPA3, Prod, FA);
    
    //hay que descargar algunas cajas
    if((SOBPA3.size()>0) && (COND_DESARMA))
    {
        vector<Bunch_cajas> BCS;
        BCS.clear();
        
        for(unsigned int p=0; p<NITERDES; p++)
        {
            CamAux2.clear();
            vector<Bunch_cajas> BCS_ITER;
            BCS_ITER.clear();
            vector<Pallet> PalletsReducidos;
            PalletsReducidos.clear();
            
            EjecutaReduccion(i, CamAux, CamAux2, SOBPA3, PalletsReducidos, BCS_ITER, Prod, FA);
            BCS.insert(BCS.end(), BCS_ITER.begin(),BCS_ITER.end());
            
            vector<Pallet> SOBPA5;
            SOBPA5.clear();
            
            vector<Camion> CAuxF;
            CAuxF.clear();
            IterandoCargaSobrante(i, true, CamAux, CAuxF, Edges, SOBPA3, SOBPA5, Prod, FA);
         
            if(SOBPA5.size() == 0)     // logré cargar el pallet correctamente
            {
                SOBPA3.clear();
                break;
            }
            else
            {
                SOBPA3.clear();
                for(unsigned int p=0; p<SOBPA5.size(); p++)
                    SOBPA3.push_back(SOBPA5[p]);
        
                SOBPA5.clear();
            }       // end else

            //Ahora se reparan los mismos pallets en otros edges
            for(unsigned int mm=0; mm<PalletsReducidos.size(); mm++)
                FindModifyPallet(i, PalletsReducidos[mm], Edges, Prod);
        
        }   // end for niteraciones
    }       // end if chequeando si es necesario reducir cajas
    
}   // end fn cargando sobranteIND

bool CheckLoadCamionVector(Bunch_cajas BC, vector<Camion>& CamAux, vector<Producto>& Prod, int* id_pallet)
{
    for(unsigned int j=0; j<CamAux.size(); j++)
    {
        for(unsigned int i=0; i<CamAux[j].carga.size(); i++)
        {
            string ProdCODE = BC.getProduct(Prod);
            int id_prod = get_Prod_id(ProdCODE, Prod);
            int familiaCE = Prod[id_prod].get_familia();
            bool Factible1 = (familiaCE == CamAux[j].carga[i].familia);
            bool Factible2 = !CamAux[j].sobreCargaK(BC.pesoBunch(Prod), Prod);
            bool Factible3 = (CamAux[j].carga[i].get_prev_edge() == -1)&&(CamAux[j].carga[i].get_next_edge() == -1);
            bool Factible4 = CamAux[j].carga[i].cargaExtra_pallet(BC,Prod);
            
            if(Factible1 && Factible2 && Factible3 && Factible4)
            {
                CamAux[j].carga[i].BC.push_back(BC);
                CamAux[j].carga[i].OcupPesoCuad(Prod);
                *id_pallet = CamAux[j].carga[i].getid_pallet();
                CamAux[j].computePesoOcupacion();
                
                return(true);
            }
        }
    }       //end for
    *id_pallet = -1;
    return(false);
}

//recarga cajas que quedan abajo siempre y cuando sea factible hacerlo. Sólo recarga pallets que son directos, para no enturbiar asignaciones doble, así que esta fn es solo para carga en arcos PS
void RecargaBunchCajas(vector<Bunch_cajas>& CargaExtra, vector<Camion>& CamAux, vector<Camion>& CCons, vector<Edge>& Edges, vector<Producto>& Prod, vector<Bunch_cajas>& Sobrante, vector<int>& palletsR)
{
    Sobrante.clear();
    palletsR.clear();
    
    int cont=0;
    for(unsigned int k=0; k<CargaExtra.size(); k++)
    {
        if(CargaExtra[k].getCajas() > 0)
        {
            cont++;
            int id_pallet;
            bool cond = CheckLoadCamionVector(CargaExtra[k], CamAux, Prod, &id_pallet);
            
            if((!cond)&&(CCons.size()>0))
                cond = CheckLoadCamionVector(CargaExtra[k], CCons, Prod, &id_pallet);
            if(!cond)
                Sobrante.push_back(CargaExtra[k]);
            else
                palletsR.push_back(id_pallet);
        }
    }       // end for
}

// cargando sobrante, que no pude cargar en arcos directos (planta-sucursal), i es el id del edge
// sólo trata de cargar el primer elemento de sobrante SOBPA, si aparecen más los trata de cargar en una nueva llamada a la función
// considera además de los camiones directos, los camiones consolidados y los pallets que se cargan en esta planta, dejando invariantes los que vienen de una planta previa
void Cargando_sobranteIND_PS(int i, vector<Camion>& CamAux, vector<Camion>& CCons, vector<Edge>& Edges, vector<Pallet>& SOBPA, vector<Pallet>& SOBPA3, vector<Producto>& Prod, vector<Familia>& FA)
{
    vector<Pallet> SOBPA2;
    SOBPA2.clear();
    SOBPA2 = SOBPA;
    
    IterandoCargaSobrante(i, false, CamAux, CCons, Edges, SOBPA2, SOBPA3, Prod, FA);
    
    //hay que descargar algunas cajas //CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCOMPLETARRRRRRRRRRRRRRRRRRRR
    if((SOBPA3.size()>0)&&(COND_DESARMA))
    {
        vector<Bunch_cajas> BCS;
        BCS.clear();
        
        for(unsigned int p=0; p<NITERDES; p++)
        {
            vector<Bunch_cajas> BCS_ITER;
            BCS_ITER.clear();
            vector<Pallet> PalletsReducidos;
            PalletsReducidos.clear();
            EjecutaReduccion(i, CamAux, CCons, SOBPA3, PalletsReducidos, BCS_ITER, Prod, FA);
            
            BCS.insert(BCS.end(), BCS_ITER.begin(),BCS_ITER.end());
            
            vector<Pallet> SOBPA5;
            SOBPA5.clear();
            
            vector<Camion> CAuxF;
            CAuxF.clear();
            IterandoCargaSobrante(i, false, CamAux, CCons, Edges, SOBPA3, SOBPA5, Prod, FA);
            
            if(SOBPA5.size() == 0)     // logré cargar el pallet correctamente
            {
                SOBPA3.clear();
                break;
            }
            else
            {
                SOBPA3.clear();
                for(unsigned int p=0; p<SOBPA5.size(); p++)
                    SOBPA3.push_back(SOBPA5[p]);
        
                SOBPA5.clear();
            }       // end else

            //Ahora se reparan los mismos pallets en otros edges
            for(unsigned int mm=0; mm<PalletsReducidos.size(); mm++)
                FindModifyPallet(i, PalletsReducidos[mm], Edges, Prod);
            
        }   // end for niteraciones

        //Ahora se trata de cargar los Bunch_Cajas resultantes en algún lugar factible
        vector<Bunch_cajas> BCS2;
        vector<int> PalletID;
        BCS2.clear();
        RecargaBunchCajas(BCS, CamAux, CCons, Edges, Prod, BCS2, PalletID);
        
    }       // end if chequeando si es necesario reducir cajas

}   // end fn cargando sobranteIND_PS

// función que realiza todo el proceso de carga: cajas a pallets y pallets a camiones
void Cargando_pallets2R(Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, vector<Producto>& Prod, vector<Edge_parts>& SOB, vector<Pallet>& SOBP, vector<Familia>& FA, int* IdLastCamion)
{
    size_t NF = FA.size();
    int PalletN = 1;
    int CamN = 1;
    
    //mantiene los sobrantes por arco, se va borrando cada vez que se vueleve a cargar un arco
    vector<Edge_parts> SOBA;
    vector<Pallet> SOBPA;
  
    for(unsigned int i=0; i<Edges.size(); i++)
    {
    // ++++++++++++++++registro primero todos los arcos que son Planta a Planta ++++++++++++++++++++++++++++++++++++
        if((Nodes[Edges[i].v()].get_type() == 1) && (Nodes[Edges[i].w()].get_type() == 1))
        {
            cout << "Procesando arco (" << Nodes[Edges[i].v()].get_cod() << ":" << Nodes[Edges[i].w()].get_cod() << ")" << endl;
            // existe flujo en dos tramos y envuelve este movimiento planta a planta
            if(Edges[i].x2.size() > 0)
            {
                set<int> destinos;
                set<int>::iterator it;
                Edges[i].findDestinos(Edges[i].x2, destinos);
                
                Edges[i].Carga2.resize(Edges[i].Carga2.size() + destinos.size());
                Edges[i].Camiones2.resize(Edges[i].Camiones2.size() + destinos.size());
                for (it=destinos.begin(); it!=destinos.end(); ++it)
                {
                    long j = distance(destinos.begin(), it);
                    vector<Camion> caux;
                    vector<vector<Pallet>> CargaAux;
                
                    int LastPall = Edges[i].cargaPalletsProductos(PalletN, Prod, Edges[i].x2, Edges[i].z2, Edges[i].Carga2[j], -1, *it, -1, SOBA,FA);
                    
                    SOB.insert(SOB.end(), SOBA.begin(), SOBA.end());
                    
                    int LastCam = Edges[i].LoadCamionesEdge(CamN,caux,Edges[i].Camiones2[j], Edges[i].Carga2[j], CargaAux, Edges[i].y2, Prod, Nodes, -1, *it,SOBPA, false,FA);
                    
                    vector<Pallet> SOBPA4;
                    SOBPA4.clear();
                    if(SOBPA.size()>0)
                    {
                        vector<Pallet> SOBPA3;
                        SOBPA3.clear();
                            
                        Cargando_sobranteIND_PP(i,Edges[i].Camiones2[j],Edges,SOBPA,SOBPA3,Prod,FA);
                        SOBPA4.insert(SOBPA4.end(), SOBPA3.begin(), SOBPA3.end());
                    }
                    
                    SOBP.insert(SOBP.end(), SOBPA4.begin(), SOBPA4.end());

                    PalletN = LastPall;
                    CamN = LastCam;
                        
                    Edges[*it].Carga2.push_back(Edges[i].Carga2[j]);
                    for(unsigned int m=0; m<Edges[*it].Carga2.back().size(); m++)
                        for(unsigned int k=0; k<Edges[*it].Carga2.back()[m].size(); k++)
                            Edges[*it].Carga2.back()[m][k].change_adjacents(i,-1);
                        
                    Edges[*it].Camiones2.push_back(Edges[i].Camiones2[j]);
                    for(unsigned int m=0; m<Edges[*it].Camiones2.back().size(); m++)
                    {
                        Edges[*it].Camiones2.back()[m].change_adjacents(i,-1);
                        if(Edges[*it].Camiones2.back()[m].carga.size()>0)
                            for(unsigned int p=0; p<Edges[*it].Camiones2.back()[m].carga.size(); p++)
                                Edges[*it].Camiones2.back()[m].carga[p].change_adjacents(i,-1);
                                
                        Edges[*it].Camiones2.back()[m].Fresco1capa = TraeFresco(Edges[*it].Camiones2.back()[m],FA);
                    }
                    
                }   // end for destinos
 
            }       // chequeando size de x2

            // existe flujo en dos tramos y envuelve este movimiento planta a planta
            
             if(Edges[i].xr.size() > 0)
             {
                 set<int> destinosR;
                 set<int>::iterator itR;
                 Edges[i].findDestinos(Edges[i].xr, destinosR);
            
                 Edges[i].CargaR.resize(Edges[i].CargaR.size() + destinosR.size());
                 for (itR=destinosR.begin(); itR!=destinosR.end(); ++itR)
                 {
                     long j = distance(destinosR.begin(),itR);
                     vector<Camion> caux;
                     vector<vector<Pallet>> CargaAux;
                     
                     int LastPall = Edges[i].cargaPalletsProductos(PalletN, Prod, Edges[i].xr, Edges[i].zr, Edges[i].CargaR[j], -1, *itR,-1, SOBA,FA);
                    
                     SOB.insert(SOB.end(), SOBA.begin(), SOBA.end());
                     PalletN = LastPall;
                     
            // copiando atributos en arco PLanta-Sucursal asociado
                    Edges[*itR].CargaR.push_back(Edges[i].CargaR[j]);
                    for(unsigned int l=0; l<Edges[*itR].CargaR.back().size(); l++)
                        for(unsigned int m=0; m<Edges[*itR].CargaR.back()[l].size(); m++)
                            Edges[*itR].CargaR.back()[l][m].change_adjacents(i,-1);
                     
                 }      // end for destinos
                 
                 //camiones de consolidacion
                 vector<vector<Pallet>> CargaConsolidada;
                 Edges[i].consolida_carga(Edges[i].CargaR, CargaConsolidada,NF);
                
                 vector<Camion> caux;
                 vector<vector<Pallet>> CargaAux;
                 int LastCam = Edges[i].LoadCamionesEdge(CamN,caux,Edges[i].CamionesD, CargaConsolidada, CargaAux, Edges[i].y, Prod, Nodes, -1, -1,SOBPA, true,FA);
                 
                 vector<Pallet> SOBPA4;
                 SOBPA4.clear();
                 if(SOBPA.size()>0)
                 {
                    vector<Pallet> SOBPA3;
                    SOBPA3.clear();
                         
                    Cargando_sobranteIND_PP(i,Edges[i].CamionesD,Edges,SOBPA,SOBPA3,Prod,FA);
                    SOBPA4.insert(SOBPA4.end(), SOBPA3.begin(), SOBPA3.end());
                 }
                 
                 SOBP.insert(SOBP.end(), SOBPA4.begin(), SOBPA4.end());

                 CamN = LastCam;
    
             }       // chequeando size de xr
 
        }       // end if para pares que son planta a planta
        
    }           // end for recorrriendo todos los arcos
    
    // ++++++++++++++++ Ahora hago las asignaciones para los pares Planta - Sucursal +++++++++++++++++++++++++++++++++++++++++
    for(unsigned int i=0; i<Edges.size(); i++)
    {
        //planta a sucursal viajes normales
        if((Nodes[Edges[i].v()].get_type() == 1) && (Nodes[Edges[i].w()].get_type() == 2))
        {
            cout << "Procesando arco (" << Nodes[Edges[i].v()].get_cod() << ":" << Nodes[Edges[i].w()].get_cod() << ")" << endl;
            //cargando pallets
            if((Edges[i].x.size()>0)||(Edges[i].x2.size()>0)||(Edges[i].xr.size()>0))
            {
                
                if(Edges[i].x.size()>0)
                {
                    int LastPall = Edges[i].cargaPalletsProductos(PalletN, Prod, Edges[i].x, Edges[i].z, Edges[i].CargaD,-1,-1,-1, SOBA,FA);
                    PalletN = LastPall;
                    
                    SOB.insert(SOB.end(), SOBA.begin(), SOBA.end());
                }
                
               //consolidando carga desde todo origen (xr), y camiones desde todo origen (y2)
                vector<vector<Pallet>> CargaConsolidadaPS;
                Edges[i].consolida_carga(Edges[i].CargaR, CargaConsolidadaPS,NF);
                
                vector<Camion> CamionesConsolidadosPS;
                Edges[i].consolida_camiones(Edges[i].Camiones2, CamionesConsolidadosPS);
                
                int LastCam = Edges[i].LoadCamionesEdge(CamN,CamionesConsolidadosPS,Edges[i].CamionesD, CargaConsolidadaPS, Edges[i].CargaD, Edges[i].y, Prod, Nodes, -1, -1,SOBPA, true,FA);
                
                vector<Pallet> SOBPA4;
                SOBPA4.clear();
                if(SOBPA.size()>0)
                {
                    vector<Pallet> SOBPA3;
                    SOBPA3.clear();
                        
                    Cargando_sobranteIND_PS(i,Edges[i].CamionesD,CamionesConsolidadosPS,Edges,SOBPA,SOBPA3,Prod,FA);
                    SOBPA4.insert(SOBPA4.end(), SOBPA3.begin(), SOBPA3.end());
                }
                
                
                SOBP.insert(SOBP.end(), SOBPA4.begin(), SOBPA4.end());
                
                CamN = LastCam;
                
                Edges[i].desconsolida_camiones(Edges[i].Camiones2,CamionesConsolidadosPS);
                
            }       // end if chequeando viajes normales
            
            // ahra chequeamos primeros arcos asociados a viajes combinados P-S-S
            if(Edges[i].xs.size()>0)
            {
                set<int> destinosS;
                set<int>::iterator itS;
                Edges[i].findDestinos(Edges[i].xs, destinosS);
                
                Edges[i].CargaS.resize(Edges[i].CargaS.size() + destinosS.size());
                Edges[i].CamionesS.resize(Edges[i].CamionesS.size() + destinosS.size());
                for (itS=destinosS.begin(); itS!=destinosS.end(); ++itS)
                {
                    long j = distance(destinosS.begin(), itS);
                    vector<Camion> caux;
                    vector<vector<Pallet>> CargaAux;
                    Edges[i].CargaS[j].clear();
                
                    int LastPall = Edges[i].cargaPalletsProductos(PalletN, Prod, Edges[i].xs, Edges[i].zs, Edges[i].CargaS[j], -1, *itS, 1, SOBA,FA);
                    SOB.insert(SOB.end(), SOBA.begin(), SOBA.end());
                    
                    //vector auxiliar para cargar los de segunda etapa
                    vector<vector<Pallet>> CargaDes;
                    CargaDes.clear();
                    
                    PalletN = LastPall;
                    LastPall = Edges[i].cargaPalletsProductos(PalletN, Prod, Edges[i].xs, Edges[i].zs, CargaDes, -1, *itS, 2, SOBA,FA);
                    SOB.insert(SOB.end(), SOBA.begin(), SOBA.end());
                    
                    for(unsigned int m=0; m<CargaDes.size(); m++)
                    {
                        Edges[i].CargaS[j][m].insert(Edges[i].CargaS[j][m].end(), CargaDes[m].begin(), CargaDes[m].end());
                    }
 
                    int LastCam = Edges[i].LoadCamionesEdge(CamN,caux,Edges[i].CamionesS[j], Edges[i].CargaS[j], CargaAux, Edges[i].ys, Prod, Nodes, -1, *itS,SOBPA, false,FA);
        
                    vector<Pallet> SOBPA4;
                    SOBPA4.clear();
                    if(SOBPA.size()>0)
                    {
                        vector<Pallet> SOBPA3;
                        SOBPA3.clear();
                            
                        Cargando_sobranteIND_PP(i,Edges[i].CamionesS[j],Edges,SOBPA,SOBPA3,Prod,FA);
                        SOBPA4.insert(SOBPA4.end(), SOBPA3.begin(), SOBPA3.end());
                    }
                    
                    SOBP.insert(SOBP.end(), SOBPA4.begin(), SOBPA4.end());

                    PalletN = LastPall;
                    CamN = LastCam;
                    
                    // copiando atributos en arco PLanta-Sucursal asociado
                        
                    Edges[*itS].CargaS.push_back(CargaDes);     //en el arco siguiente, sólo cargo los pallets que siguen
                    for(unsigned int m=0; m<Edges[*itS].CargaS.back().size(); m++)
                        for(unsigned int k=0; k<Edges[*itS].CargaS.back()[m].size(); k++)
                            Edges[*itS].CargaS.back()[m][k].change_adjacents(i,-1);
                        
                    Edges[*itS].CamionesS.push_back(Edges[i].CamionesS[j]);
                    for(unsigned int m=0; m<Edges[*itS].CamionesS.back().size(); m++)
                    {
                        Edges[*itS].CamionesS.back()[m].change_adjacents(i,-1);
                        if(Edges[*itS].CamionesS.back()[m].carga.size()>0)
                            for(unsigned int p=0; p<Edges[*itS].CamionesS.back()[m].carga.size(); p++)
                                Edges[*itS].CamionesS.back()[m].carga[p].change_adjacents(i,-1);
                                
                        Edges[*itS].CamionesS.back()[m].Fresco1capa = TraeFresco(Edges[*itS].CamionesS.back()[m],FA);
                        //ahora descargo los pallets que se bajan
                        for(unsigned int pp=0; pp< Edges[*itS].CamionesS.back().size(); pp++)
                        {
                            vector<int> indices_bajan;
                            for(unsigned int kk=0; kk<Edges[*itS].CamionesS.back()[pp].carga.size(); kk++)
                                if(Edges[*itS].CamionesS.back()[pp].carga[kk].get_descarga() == 1)
                                    indices_bajan.push_back(kk);
                            //actualizo pallets por familia
                            for(unsigned int k=0; k<indices_bajan.size(); k++)
                            {
                                int fam = Edges[*itS].CamionesS.back()[pp].carga[indices_bajan[k]].familia;
                                Edges[*itS].CamionesS.back()[pp].PalletsPorFamilia[fam]--;
                            }
                            DeleteAll(Edges[*itS].CamionesS.back()[pp].carga, indices_bajan);
                            indices_bajan.clear();
                            Edges[*itS].CamionesS.back()[pp].computePesoOcupacion();
                        }
                    }
         
                }   // end for destinos
            }
   
        }   // end if nodo planta - sucursal
        
    }       // end for edges

    *IdLastCamion = CamN - 1;
}       // end function Cargando_pallets2R

//*******++++*********++++++++++++++++++*************************************************

// FN PRELIMINAR PARA CALCULAR INDICADOR ASOCIADO A CARGAS DE CAMIONES EN NODOS INTERMEDIOS, true si camión es cargado en ambas plantas, false si se carga solo en la primera
bool Camion2(Camion CA, vector<Edge>& Edges)
{
    int ii,jj;
    int second_id = CA.get_nxt();
    if(second_id != -1)
    {
        Edges[second_id].searchCamion(CA.get_idc(), &ii, &jj);
        if(CA.carga.size() == Edges[second_id].Camiones2[ii][jj].carga.size())
        {
            int cond = 0;
            for(unsigned int k=0; k<CA.carga.size(); k++)
            {
                for(unsigned int j=0; j<Edges[second_id].Camiones2[ii][jj].carga.size(); j++)
                {
                    if(CA.carga[k].getid_pallet() == Edges[second_id].Camiones2[ii][jj].carga[j].getid_pallet())
                    {
                        cond ++;
                        break;
                    }
                }
            }
            if(cond == CA.carga.size())
                return(false);
        }
        else
            return(true);
    }
    return (false);
}

//function for printing the elements in a list
void showlist(list <int> g)
{
    list <int> :: iterator it;
    for(it = g.begin(); it != g.end(); ++it)
        cout << '\t' << *it;
    cout << '\n';
}

//function for printing the elements in a list
void showvec(vector <int> g)
{
    for(unsigned int i = 0; i < g.size(); ++i)
        cout << '\t' << g[i];
    cout << '\n';
}

void ProgramaItinerarioCamiones(Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, int LastCamID)
{
    bool cond = true;
    vector<int> CamionesMarcados0;
    CamionesMarcados0.clear();
    vector<int> CamionesMarcados1;
    int cont =0;
    
    while(cond)
    {
        for(unsigned int kk=0; kk<Nodes.size(); kk++)
        {
            Nodes[kk].Schedule0.clear();
            Nodes[kk].Schedule1.clear();
            Nodes[kk].Schedule2.clear();
            Nodes[kk].Schedule3.clear();
        }
                
 //       showvec(CamionesMarcados0);
 //       showvec(CamionesMarcados1);
        
        CamionesMarcados1.clear();
    // seteando schedules de nodos sucursales
        for(unsigned int kk=0; kk<Nodes.size(); kk++)
            G.FillCamionesSucursal(kk,Edges,Nodes,CamionesMarcados0);
        G.AjustaItinerarios(Edges,Nodes);
            G.FillCamionesPlanta(Edges,Nodes);
            for(unsigned int pp=0; pp<Nodes.size(); pp++)
                Nodes[pp].OrdenaProgramaciones();
       
        //revisando programación y marcando nodos inadecuados
        
        for(unsigned int pp=1; pp<=LastCamID; pp++)
        {
            int pos;
            int PID = G.NodeID_planta1(pp, Edges, Nodes);
            int Sch = Nodes[PID].findCamionSchedule(pp,&pos);
                        
            if(Sch>=1)
                CamionesMarcados1.push_back(pp);
        }
        
        if(CamionesMarcados1.empty())
            break;
        
        CamionesMarcados0.insert(CamionesMarcados0.end(), CamionesMarcados1.begin(), CamionesMarcados1.end());
        
        cont++;
//        if((cont > 1)&&(CamionesMarcados.size() < 3))
        if(cont == NITERPROG)
            break;
       
    }       // end while
    //fixing the schedules de camiones que quedan marcados
    list <int> :: iterator it;
    for(unsigned int i = 0; i < CamionesMarcados1.size(); i++)
    {
        vector<int> camPos = G.NodesID(i, Edges, Nodes);
        int pos2;
        //se ajusta la planta inicial a la ventana de día 0
        int sch = Nodes[camPos[0]].findCamionSchedule(i, &pos2);
        if(sch == 1)
        {
            double DeltaT = Nodes[camPos[0]].Schedule1[pos2].programacion - Nodes[camPos[0]].getVentanaB();
            for(unsigned int j=0; j<camPos.size(); j++)
                Nodes[camPos[j]].Reasigna_schedule(DeltaT, *it);
        }
    }
    
    //finalmente programo los camiones alimentadores
    G.FillCamionesFeeder(Edges,Nodes);
    //ordenando itinerarios finales
    for(unsigned int pp=0; pp<Nodes.size(); pp++)
        Nodes[pp].OrdenaProgramaciones();
    
}       // end fn

struct ProductoCajasClasif
{
    string codigo_producto;      // codigo producto
    string clasificacion;        // primario o interplanta
    bool super;                 //true if super, false si no
    int ncajas;    // número de cajas para ese ítem
    vector<int> cam_id;     //para computar un tiempo
    int nioS;               // vale node_id nodo origen de bucn de producto
    int nidS;               //vale node_id destino de bunch de producto

    ProductoCajasClasif(string cp, string cl, bool sp, int nc, int cid, int nos, int nds): codigo_producto(cp), clasificacion(cl), super(sp), ncajas(nc), nioS(nos), nidS(nds) {cam_id.push_back(cid);}
    ProductoCajasClasif(string cp, string cl, bool sp, int nc, vector<int>& cam_idS, int nos, int nds): codigo_producto(cp), clasificacion(cl), super(sp), ncajas(nc), nioS(nos), nidS(nds) {cam_id = cam_idS;}
};

//encuentra el id del edge dados los nodos origen y destino
int edge_ID(string NO, string ND, vector<Edge>& Edges, vector<Node>& Nodes)
{
    for(unsigned int k=0; k<Edges.size(); k++)
    {
        if((Nodes[Edges[k].v()].get_cod() == NO)&&(Nodes[Edges[k].w()].get_cod() == ND))
            return(k);
    }
    return(-1);
}

vector<Pallet> DeltaDemanda(Camion& Cam21, Camion& Cam22, Graph& G, vector<Edge>& Edges, vector <Node>& Nodes)
{
    vector<Pallet> DeltaD;
    for(unsigned int k=0; k<Cam22.carga.size();k++)
    {
        bool cond = false;
        for(unsigned int j=0; j<Cam21.carga.size(); j++)
        {
            if(Cam22.carga[k].getid_pallet() == Cam21.carga[j].getid_pallet())
                cond = true;
        }
        if(!cond)
            DeltaD.push_back(Cam22.carga[k]);
    }
    return(DeltaD);
}       // end fn DeltaDemanda


//fn que entrega la desagregación de archivo SAP transportes detalle, para el delta demanda de los y2 en planta intermedia
vector<double> palletsPorSectorGeneric(vector<Pallet>& Pallets, vector<Familia>& FA, vector<Producto>& Prod)
{
    vector<double> Aux; //acá guardo la secuencia de pallets clasificados
    Aux.clear();
    for(unsigned int i=0; i<13; i++)
        Aux.push_back(0);
    for(unsigned int k=0; k<Pallets.size(); k++)
    {
        if(FA[Pallets[k].familia].congelado)       //se guarda en un orden
        {
            double PolloCongelado = Pallets[k].ParteSector("Pollo", Prod);
            double PavoCongelado = Pallets[k].ParteSector("Pavo", Prod);
            double CerdoCongelado = Pallets[k].ParteSector("Cerdo", Prod);
            double ElaboradoCongelado = Pallets[k].ParteSector("Elaborado", Prod);
            double HortalizasCongelado = Pallets[k].ParteSector("Hortalizas y Frutas", Prod);
            double SalmonCongelado = Pallets[k].ParteSector("Salmón", Prod);
            
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
            double Cecina = Pallets[k].ParteSector("Cecina", Prod);
            double PolloFresco = Pallets[k].ParteSector("Pollo", Prod);
            double PavoFresco = Pallets[k].ParteSector("Pavo", Prod);
            double CerdoFresco = Pallets[k].ParteSector("Cerdo", Prod);
            double SalmonFresco = Pallets[k].ParteSector("Salmón", Prod);
            
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

//fn que entrega la desagregación de archivo SAP transportes detalle, para el delta demanda de los y2 en planta intermedia
vector<double> kilosPorSectorGeneric(vector<Pallet>& Pallets, vector<Familia>& FA, vector<Producto>& Prod)
{
    vector<double> Aux; //acá guardo la secuencia de pallets clasificados
    Aux.clear();
    for(unsigned int i=0; i<13; i++)
        Aux.push_back(0);
    for(unsigned int k=0; k<Pallets.size(); k++)
    {
        if(FA[Pallets[k].familia].congelado)       //se guarda en un orden
        {
            double PolloCongelado = Pallets[k].ParteSectorKilos("Pollo", Prod);
            double PavoCongelado = Pallets[k].ParteSectorKilos("Pavo", Prod);
            double CerdoCongelado = Pallets[k].ParteSectorKilos("Cerdo", Prod);
            double ElaboradoCongelado = Pallets[k].ParteSectorKilos("Elaborado", Prod);
            double HortalizasCongelado = Pallets[k].ParteSectorKilos("Hortalizas y Frutas", Prod);
            double SalmonCongelado = Pallets[k].ParteSectorKilos("Salmón", Prod);
            
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
            double Cecina = Pallets[k].ParteSectorKilos("Cecina", Prod);
            double PolloFresco = Pallets[k].ParteSectorKilos("Pollo", Prod);
            double PavoFresco = Pallets[k].ParteSectorKilos("Pavo", Prod);
            double CerdoFresco = Pallets[k].ParteSectorKilos("Cerdo", Prod);
            double SalmonFresco = Pallets[k].ParteSectorKilos("Salmón", Prod);
            
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


void salidaTransporteDetalle(ofstream& SalidaDetalle, Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, vector<Producto>& Prod, int lastCamID, vector<Familia>& FA)
{
    SalidaDetalle << "CorrCamion;Nodo;Fecha-HoraLlegada(segundos);Fecha-HoraSalida(segundos);NoEntregas;Po C;Cer C;El C;Sa C;Pa C;Ho C;Po F;Ce F;Pa F;Ce;Sa F;Vin;Vacun;" << endl;
    SalidaDetalle << setiosflags(ios_base::fixed);
    SalidaDetalle << setprecision(0);
    
    for(unsigned int i=1; i<=lastCamID; i++)
    {
        vector<int> NodosAsoc = G.NodesID(i,Edges,Nodes);
        for(unsigned int j=0; j<NodosAsoc.size(); j++)
        {
            int pos;
            int sch = Nodes[NodosAsoc[j]].findCamionSchedule(i, &pos);
            if(j==0)    //análisis PLanta 1
            {
                int pos1, pos2;
                int edge_id = edge_ID(Nodes[NodosAsoc[j]].get_cod(), Nodes[NodosAsoc[j+1]].get_cod(),Edges, Nodes);
                int LocCam = Edges[edge_id].searchCamionTotal(i, &pos1, &pos2);
                vector<double> DesagregacionPallets;
                
                //carga de este camión en planta1
                if(LocCam == 1) //está en CamionesD
                    DesagregacionPallets = Edges[edge_id].CamionesD[pos1].palletsPorSector(Edges[edge_id].CamionesD[pos1].carga, FA, Prod);
                else if(LocCam == 2) //está en Camiones2
                    DesagregacionPallets = Edges[edge_id].Camiones2[pos1][pos2].palletsPorSector(Edges[edge_id].Camiones2[pos1][pos2].carga, FA, Prod);
                else if(LocCam == 3) //está en CamionesS
                    DesagregacionPallets = Edges[edge_id].CamionesS[pos1][pos2].palletsPorSector(Edges[edge_id].CamionesS[pos1][pos2].carga,FA, Prod);
               
                time_t tinicio = 0;
                time_t tfinal = 0;
                //ahora obtengo los tiempos del schedule
                if(sch == 0)
                    tinicio = Nodes[NodosAsoc[j]].Schedule0[pos].programacion;
                else if(sch == 1)
                    tinicio = Nodes[NodosAsoc[j]].Schedule1[pos].programacion;
                else if(sch == 2)
                    tinicio = Nodes[NodosAsoc[j]].Schedule2[pos].programacion;
                else if(sch == 3)
                    tinicio = Nodes[NodosAsoc[j]].Schedule3[pos].programacion;
                
                tfinal = tinicio + Nodes[NodosAsoc[j]].getTcarga()*3600;
                SalidaDetalle << i << ";" << Nodes[NodosAsoc[j]].get_cod() << ";" << tinicio << ";" << tfinal << ";X;";
                for(unsigned int pp=0; pp<DesagregacionPallets.size(); pp++)
                {
                    SalidaDetalle << setprecision(1);
                    if(DesagregacionPallets[pp] > 0.0)
                        SalidaDetalle << DesagregacionPallets[pp] << ";";
                    else
                        SalidaDetalle << ";";       //  DUDA EN ESTE FORMATO PREGUNTAR A RAUL
                }
                SalidaDetalle << ";;" << endl;
            }   // end j==0 planta inicial de cualquier viaje
            else if((j==1)&&(NodosAsoc.size()==2))      //casi todos los casos
            {
                int pos1, pos2;
                int edge_id = edge_ID(Nodes[NodosAsoc[j-1]].get_cod(), Nodes[NodosAsoc[j]].get_cod(),Edges, Nodes);
                int LocCam = Edges[edge_id].searchCamionTotal(i, &pos1, &pos2);
               
                time_t tinicio = 0;
                time_t tfinal = 0;
                //ahora obtengo los tiempos del schedule
                if(sch == 0)
                    tinicio = Nodes[NodosAsoc[j]].Schedule0[pos].programacion;
                else if(sch == 1)
                    tinicio = Nodes[NodosAsoc[j]].Schedule1[pos].programacion;
                else if(sch == 2)
                    tinicio = Nodes[NodosAsoc[j]].Schedule2[pos].programacion;
                else if(sch == 3)
                    tinicio = Nodes[NodosAsoc[j]].Schedule3[pos].programacion;
                
                tfinal = tinicio + Nodes[NodosAsoc[j]].getTdescarga()*3600;
                SalidaDetalle << i << ";" << Nodes[NodosAsoc[j]].get_cod() << ";" << tinicio << ";" << tfinal << ";;;;;;;;;;;;;;" << endl;
            }
            else if((j==1)&&(NodosAsoc.size()==3))      //esto podría ser un Camiones2 o un CamionesS
            {
                int pos1, pos2, pos3, pos4;
                int edge_id1 = edge_ID(Nodes[NodosAsoc[j-1]].get_cod(), Nodes[NodosAsoc[j]].get_cod(),Edges, Nodes);
                int edge_id2 = edge_ID(Nodes[NodosAsoc[j]].get_cod(), Nodes[NodosAsoc[j+1]].get_cod(),Edges, Nodes);
                int LocCam1 = Edges[edge_id1].searchCamionTotal(i, &pos1, &pos2);
                int LocCam2 = Edges[edge_id2].searchCamionTotal(i, &pos3, &pos4);
            
                time_t tinicio = 0;
                time_t tfinal = 0;
                //ahora obtengo los tiempos del schedule
                if(sch == 0)
                    tinicio = Nodes[NodosAsoc[j]].Schedule0[pos].programacion;
                else if(sch == 1)
                    tinicio = Nodes[NodosAsoc[j]].Schedule1[pos].programacion;
                else if(sch == 2)
                    tinicio = Nodes[NodosAsoc[j]].Schedule2[pos].programacion;
                else if(sch == 3)
                    tinicio = Nodes[NodosAsoc[j]].Schedule3[pos].programacion;
                    
                double DeltaOper;
                if(Nodes[NodosAsoc[j]].get_type() == 1)
                    DeltaOper = Nodes[NodosAsoc[j]].getTcarga()*3600;
                else
                    DeltaOper = Nodes[NodosAsoc[j]].getTdescarga()*3600;
                tfinal = tinicio + DeltaOper;
                    
                    SalidaDetalle << setprecision(0);
                SalidaDetalle << i << ";" << Nodes[NodosAsoc[j]].get_cod() << ";" << tinicio << ";" << tfinal << ";";
                if(Nodes[NodosAsoc[j]].get_type() == 1)
                {
                  
                    vector<Pallet> DD = DeltaDemanda(Edges[edge_id1].Camiones2[pos1][pos2], Edges[edge_id2].Camiones2[pos3][pos4], G, Edges, Nodes);
                    vector<double> DesagregacionPallets = palletsPorSectorGeneric(DD, FA, Prod);
                   
                    SalidaDetalle << "X;";
                    for(unsigned int pp=0; pp<DesagregacionPallets.size(); pp++)
                    {
                        SalidaDetalle << setprecision(1);
                        if(DesagregacionPallets[pp] > 0.0)
                            SalidaDetalle << DesagregacionPallets[pp] << ";";
                        else
                            SalidaDetalle << ";";
                    }
                    SalidaDetalle << ";;" << endl;
                }
                else
                    SalidaDetalle << ";;;;;;;;;;;;;" << endl;     //  DUDA EN ESTE FORMATO PREGUNTAR A RAUL
            }
            else if(j==2)
            {
                int pos1, pos2;
                int edge_id = edge_ID(Nodes[NodosAsoc[j-1]].get_cod(), Nodes[NodosAsoc[j]].get_cod(),Edges, Nodes);
                int LocCam = Edges[edge_id].searchCamionTotal(i, &pos1, &pos2);
               
                time_t tinicio = 0;
                time_t tfinal = 0;
                //ahora obtengo los tiempos del schedule
                if(sch == 0)
                    tinicio = Nodes[NodosAsoc[j]].Schedule0[pos].programacion;
                else if(sch == 1)
                    tinicio = Nodes[NodosAsoc[j]].Schedule1[pos].programacion;
                else if(sch == 2)
                    tinicio = Nodes[NodosAsoc[j]].Schedule2[pos].programacion;
                else if(sch == 3)
                    tinicio = Nodes[NodosAsoc[j]].Schedule3[pos].programacion;
                
                tfinal = tinicio + Nodes[NodosAsoc[j]].getTdescarga()*3600;
                SalidaDetalle << i << ";" << Nodes[NodosAsoc[j]].get_cod() << ";" << tinicio << ";" << tfinal << ";;;;;;;;;;;;;;" << endl;
            }
            
        }   //end for recorriendo nodos asociados
        
    }   // end for recorriendo camiones
}

void salidaVentaTraslado(ofstream& SalidaVenta, Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, vector<Producto>& Prod)
{
    SalidaVenta << "Clasificacion;Material;CodeNorigen;CodeNdestino;Supermercado;Ncajas;Suministro(solo para traslados);" << endl;
    SalidaVenta << setiosflags(ios_base::fixed);
    SalidaVenta << setprecision(0);
    
    //agregación final para diferentes tipos de camiones (CamionesD, Camiones2, CamionesS)
    vector<ProductoCajasClasif> PC2;
    PC2.clear();
    
    for(unsigned int i=0; i<Edges.size(); i++)
    {
        vector<ProductoCajasClasif> PC;
        vector<int> Cam_id;                 //camiones que llegan al destino asi
        PC.clear();
        Cam_id.clear();
        
        for(unsigned int j=0; j<Edges[i].CamionesD.size(); j++)
        {
            if((Nodes[Edges[i].v()].get_type() == 1)&&(Nodes[Edges[i].w()].get_type() == 1))    //camiones interplanta
            {
                for(unsigned int k=0; k<Edges[i].CamionesD[j].carga.size(); k++)
                {
                    for(unsigned int l=0; l<Edges[i].CamionesD[j].carga[k].BC.size(); l++)
                    {
                        string PrBunch = Edges[i].CamionesD[j].carga[k].BC[l].getProduct(Prod);
                        string Clasif = Edges[i].CamionesD[j].ClasifCamion;
  //                      bool super = Edges[i].CamionesD[j].carga[k].BC[l].getSuper();
                        bool super = false; //camiones interplanta no hay discriminación por super
                        bool cond = false;
                        
                        for(unsigned m=0; m<PC.size(); m++)
                        {
                            if((PC[m].codigo_producto == PrBunch)&&(PC[m].clasificacion == Clasif)&&(PC[m].nioS == Edges[i].v())&&(PC[m].nidS == Edges[i].w()))
                            {
                                cond = true;
                                PC[m].ncajas += Edges[i].CamionesD[j].carga[k].BC[l].getCajas();
                                PC[m].cam_id.push_back(Edges[i].CamionesD[j].get_idc());
                            }
                        }
                        if(!cond)
                            PC.push_back(ProductoCajasClasif(PrBunch,Clasif,super,Edges[i].CamionesD[j].carga[k].BC[l].getCajas(),Edges[i].CamionesD[j].get_idc(),Edges[i].v(), Edges[i].w()));
                   
                    }   // end for recorriendo BCs
        
                }       // end for recorriendo pallets
                
            }       // end if mirando camiones interplanta, genera par O-D de tipo P-S
            else if((Nodes[Edges[i].v()].get_type() == 1)&&(Nodes[Edges[i].w()].get_type() == 2))   //viajes directos
            {
                for(unsigned int k=0; k<Edges[i].CamionesD[j].carga.size(); k++)
                {
                    for(unsigned int l=0; l<Edges[i].CamionesD[j].carga[k].BC.size(); l++)
                    {
                        string PrBunch = Edges[i].CamionesD[j].carga[k].BC[l].getProduct(Prod);
                        string Clasif = Edges[i].CamionesD[j].ClasifCamion;
                        bool super = Edges[i].CamionesD[j].carga[k].BC[l].getSuper();
                        bool cond = false;
                        
                        for(unsigned m=0; m<PC.size(); m++)
                        {
                            if((PC[m].codigo_producto == PrBunch)&&(PC[m].clasificacion == Clasif)&&(PC[m].super == super)&&(PC[m].nioS == Edges[i].v())&&(PC[m].nidS == Edges[i].w()))
                            {
                                cond = true;
                                PC[m].ncajas += Edges[i].CamionesD[j].carga[k].BC[l].getCajas();
                                PC[m].cam_id.push_back(Edges[i].CamionesD[j].get_idc());
                            }
                        }
                        if(!cond)
                            PC.push_back(ProductoCajasClasif(PrBunch,Clasif,super,Edges[i].CamionesD[j].carga[k].BC[l].getCajas(),Edges[i].CamionesD[j].get_idc(),Edges[i].v(), Edges[i].w()));

                    }   // end for recorriendo BCs
                        
                }       // end for recorriendo pallets
            }   //end else if
            
        }           // end for CamionesD
        
        for(unsigned int j=0; j<Edges[i].Camiones2.size(); j++)
        {
            for(unsigned int s=0; s<Edges[i].Camiones2[j].size(); s++)
            {
                for(unsigned int k=0; k<Edges[i].Camiones2[j][s].carga.size(); k++)
                {
                    for(unsigned int l=0; l<Edges[i].Camiones2[j][s].carga[k].BC.size(); l++)
                    {
                        string PrBunch = Edges[i].Camiones2[j][s].carga[k].BC[l].getProduct(Prod);
                        string Clasif = Edges[i].Camiones2[j][s].ClasifCamion;
                        bool super = Edges[i].Camiones2[j][s].carga[k].BC[l].getSuper();
                        
                        int nSF = -1;
                        if((Nodes[Edges[i].v()].get_type() == 1)&&(Nodes[Edges[i].w()].get_type() == 1))
                        {
                            super = false;      //planta-planta no cuenta super
                            nSF = Edges[Edges[i].Camiones2[j][s].carga[k].get_next_edge()].w();
                            bool cond = false;
                            for(unsigned m=0; m<PC.size(); m++)
                            {
                                if((PC[m].codigo_producto == PrBunch)&&(PC[m].clasificacion == Clasif)&&(PC[m].nidS == nSF)&&(PC[m].nioS == Edges[i].v()))
                                {
                                    cond = true;
                                    PC[m].ncajas += Edges[i].Camiones2[j][s].carga[k].BC[l].getCajas();
                                    PC[m].cam_id.push_back(Edges[i].Camiones2[j][s].get_idc());
                                }
                            }
                            if(!cond)
                                PC.push_back(ProductoCajasClasif(PrBunch,Clasif,super,Edges[i].Camiones2[j][s].carga[k].BC[l].getCajas(), Edges[i].Camiones2[j][s].get_idc(),Edges[i].v(),nSF));
                        }   // end if P-P que genera par P-S
                        else if((Nodes[Edges[i].v()].get_type() == 1)&&(Nodes[Edges[i].w()].get_type() == 2))
                        {
                            nSF = Edges[i].w();
                            if(Edges[i].Camiones2[j][s].carga[k].get_prev_edge() == -1)     //origen del pallet es P2
                            {
                                bool cond = false;
                                for(unsigned m=0; m<PC.size(); m++)
                                {
                                    if((PC[m].codigo_producto == PrBunch)&&(PC[m].clasificacion == Clasif)&&(PC[m].super == super)&&(PC[m].nidS == nSF)&&(PC[m].nidS == nSF)&&(PC[m].nioS == Edges[i].v()))
                                    {
                                        cond = true;
                                        PC[m].ncajas += Edges[i].Camiones2[j][s].carga[k].BC[l].getCajas();
                                        PC[m].cam_id.push_back(Edges[i].Camiones2[j][s].get_idc());
                                    }
                                }
                                if(!cond)
                                    PC.push_back(ProductoCajasClasif(PrBunch,Clasif,super,Edges[i].Camiones2[j][s].carga[k].BC[l].getCajas(), Edges[i].Camiones2[j][s].get_idc(),Edges[i].v(),nSF));
                            }   //chequeando si este bunch es exclsivo de este arco o no
                            
                        }   // end else if P2-S que genera par P2-S
                    }   // end for checking BC
                   
                }       // end for checking carga
            }
        }           // end for Camiones2
        
        for(unsigned int j=0; j<Edges[i].CamionesS.size(); j++)
        {
            for(unsigned int s=0; s<Edges[i].CamionesS[j].size(); s++)
            {
                for(unsigned int k=0; k<Edges[i].CamionesS[j][s].carga.size(); k++)
                {
                    for(unsigned int l=0; l<Edges[i].CamionesS[j][s].carga[k].BC.size(); l++)
                    {
                        string PrBunch = Edges[i].CamionesS[j][s].carga[k].BC[l].getProduct(Prod);
                        string Clasif = Edges[i].CamionesS[j][s].ClasifCamion;
                        bool super = Edges[i].CamionesS[j][s].carga[k].BC[l].getSuper();
                        
                        int nSF = -1;
                        if((Nodes[Edges[i].v()].get_type() == 1)&&(Nodes[Edges[i].w()].get_type() == 2))
                        {
                            if(Edges[i].CamionesS[j][s].carga[k].get_descarga() == 1)
                            {
                                nSF = Edges[i].w();
                                bool cond = false;
                                for(unsigned m=0; m<PC.size(); m++)
                                {
                                    if((PC[m].codigo_producto == PrBunch)&&(PC[m].clasificacion == Clasif)&&(PC[m].super == super)&&(PC[m].nidS == nSF)&&(PC[m].nioS == Edges[i].v()))
                                    {
                                        cond = true;
                                        PC[m].ncajas += Edges[i].CamionesS[j][s].carga[k].BC[l].getCajas();
                                        PC[m].cam_id.push_back(Edges[i].CamionesS[j][s].get_idc());
                                    }
                                }
                                if(!cond)
                                    PC.push_back(ProductoCajasClasif(PrBunch,Clasif,super,Edges[i].CamionesS[j][s].carga[k].BC[l].getCajas(), Edges[i].CamionesS[j][s].get_idc(),Edges[i].v(),nSF));
                            }// DESCARGA EN S1
                            else if(Edges[i].CamionesS[j][s].carga[k].get_descarga()==2)
                            {
                                nSF = Edges[Edges[i].CamionesS[j][s].carga[k].get_next_edge()].w();
                                bool cond = false;
                                for(unsigned m=0; m<PC.size(); m++)
                                {
                                    if((PC[m].codigo_producto == PrBunch)&&(PC[m].clasificacion == Clasif)&&(PC[m].super == super)&&(PC[m].nidS == nSF)&&(nSF == Edges[i].v()))
                                    {
                                        cond = true;
                                        PC[m].ncajas += Edges[i].CamionesS[j][s].carga[k].BC[l].getCajas();
                                        PC[m].cam_id.push_back(Edges[i].CamionesS[j][s].get_idc());
                                    }
                                }
                                if(!cond)
                                    PC.push_back(ProductoCajasClasif(PrBunch,Clasif,super,Edges[i].CamionesS[j][s].carga[k].BC[l].getCajas(), Edges[i].CamionesS[j][s].get_idc(),Edges[i].v(),nSF));
                            }
                            
                        }   // end if P-S primer tramo y con eso construyo P-S1 y P-S2
                        
                    }   // end for checking BC
                }
            }       // end for checking carga
        }           // end for CamionesS
        
        PC2.insert(PC2.end(), PC.begin(), PC.end());
  
    }   // recorriendo Edges
    
    // última posible agregación
    vector<ProductoCajasClasif> PC3;
    PC3.clear();

     for(unsigned int p=0; p<PC2.size(); p++)
     {
         bool cond = false;
         bool condSuper1 = true;
         if((Nodes[PC2[p].nioS].get_type() == 1)&&(Nodes[PC2[p].nidS].get_type() == 1))
             condSuper1 = false;
             
         for(unsigned m=0; m<PC3.size(); m++)
         {
             bool condSuper2 = true;
                 if(PC2[p].super != PC3[m].super)
                     condSuper2 = false;
             
             bool condSuperF = condSuper1 && condSuper2;
             
             if((PC2[p].codigo_producto == PC3[m].codigo_producto)&&(PC2[p].clasificacion == PC3[m].clasificacion)&&(condSuperF)&&(PC2[p].nioS == PC3[m].nioS)&&(PC2[p].nidS == PC3[m].nidS))
             {
                 cond = true;
                 PC3[m].ncajas += PC2[p].ncajas;
                 PC3[m].cam_id.insert(PC3[m].cam_id.end(), PC2[p].cam_id.begin(), PC2[p].cam_id.end());
             }
         }
         if(!cond)
         {
             PC3.push_back(ProductoCajasClasif(PC2[p].codigo_producto,PC2[p].clasificacion,PC2[p].super,PC2[p].ncajas, PC2[p].cam_id,PC2[p].nioS, PC2[p].nidS));
         }
     }   //fin for última agregación
     
    
    // escribiendo summary para este arco
    for(unsigned int k=0; k<PC3.size(); k++)
    {
        time_t FechaSuministro = Nodes[PC3[k].nidS].TiempoCritico(PC3[k].cam_id);
        int eid = get_Edge_id(Nodes[PC3[k].nioS].get_cod(), Nodes[PC3[k].nidS].get_cod(), Edges, Nodes);
        if(FechaSuministro == 0)
            FechaSuministro = Nodes[PC3[k].nioS].getMiddleVentana() + 3600*Edges[eid].getTViaje();

        SalidaVenta << PC3[k].clasificacion << ";" << PC3[k].codigo_producto << ";" << Nodes[PC3[k].nioS].get_cod() << ";" << Nodes[PC3[k].nidS].get_cod() << ";";
        if((PC3[k].super)&&(Nodes[Edges[eid].w()].get_type() == 2))
            SalidaVenta << "S;";
        else
            SalidaVenta << ";";
        SalidaVenta << PC3[k].ncajas << ";" << FechaSuministro << ";" << endl;
    }   // fin for escribiendo datos para este arco
    
}       //end SalidaVentaTraslado

void salidaTransportesCabecera(ofstream& SalidaCabecera, Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, vector<Familia>& FA)
{
    // computamos Visita Nodos Pickup +++++++++++++++++++++++++++++++++++++++++++++++++++++
    
    SalidaCabecera << "CorrCamion;TipoCamion;Separador;Clasificacion;RutaDescargas;" << endl;
    SalidaCabecera << setiosflags(ios_base::fixed);
    SalidaCabecera << setprecision(0);

    for(unsigned int i=0; i<Edges.size(); i++)
    {
        if((Nodes[Edges[i].v()].get_type() == 1) && (Nodes[Edges[i].w()].get_type() == 1))
        {
            if(Edges[i].CamionesD.size()>0)
            {
                string ruta = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                for(unsigned int j=0; j<Edges[i].CamionesD.size(); j++)
                {
                    SalidaCabecera << Edges[i].CamionesD[j].get_idc() << ";" << Edges[i].CamionesD[j].get_stype() << ";";
                    if(Edges[i].CamionesD[j].separador(FA))
                        SalidaCabecera << "S;";
                    else
                        SalidaCabecera << ";";
                    SalidaCabecera << Edges[i].CamionesD[j].ClasifCamion << ";" << ruta << ";" << endl;
                }           // end for j
            }       // end if CamionesD
            
            // caso de Camiones2 (planta a planta) ++++++++++++++++++++++++++++++++++
            if(Edges[i].Camiones2.size() > 0)
            {
                string ruta = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                for(unsigned int j=0; j<Edges[i].Camiones2.size(); j++)
                {
                    if(Edges[i].Camiones2[j].size()>0)
                    {
                        for(unsigned int l=0; l<Edges[i].Camiones2[j].size(); l++)
                        {
                            int next_link_id = Edges[i].Camiones2[j][l].get_nxt();
                            string ruta_final = ruta + "-" + Nodes[Edges[next_link_id].w()].get_cod();
                            
                            SalidaCabecera << Edges[i].Camiones2[j][l].get_idc() << ";" << Edges[i].Camiones2[j][l].get_stype() << ";";
                            if(Edges[i].Camiones2[j][l].separador(FA))
                                SalidaCabecera << "S;";
                            else
                                SalidaCabecera << ";";
                            
                            SalidaCabecera << Edges[i].Camiones2[j][l].ClasifCamion << ";" << ruta_final << ";" << endl;
                            
                    
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
                string ruta = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                for(unsigned int j=0; j<Edges[i].CamionesD.size(); j++)
                {
                    SalidaCabecera << Edges[i].CamionesD[j].get_idc() << ";" << Edges[i].CamionesD[j].get_stype() << ";";
                    if(Edges[i].CamionesD[j].separador(FA))
                        SalidaCabecera << "S;";
                    else
                        SalidaCabecera << ";";
                    SalidaCabecera << Edges[i].CamionesD[j].ClasifCamion << ";" << ruta << ";" << endl;
                    
                }           // end for j
            }       // end if CamionesD
            
            // caso de CamionesS (planta a sucursal) ++++++++++++++++++++++++++++++++++
            if(Edges[i].CamionesS.size() > 0)
            {
                string ruta = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                for(unsigned int j=0; j<Edges[i].CamionesS.size(); j++)
                {
                    if(Edges[i].CamionesS[j].size()>0)
                    {
                        for(unsigned int l=0; l<Edges[i].CamionesS[j].size(); l++)
                        {
                            int next_link_id = Edges[i].CamionesS[j][l].get_nxt();
                            string ruta_final = ruta + "-" + Nodes[Edges[next_link_id].w()].get_cod();
                            
                            SalidaCabecera << Edges[i].CamionesS[j][l].get_idc() << ";" << Edges[i].CamionesS[j][l].get_stype() << ";";
                            if(Edges[i].CamionesS[j][l].separador(FA))
                                SalidaCabecera << "S;";
                            else
                                SalidaCabecera << ";";
                            SalidaCabecera << Edges[i].CamionesS[j][l].ClasifCamion << ";" << ruta_final << ";" << endl;

                        }       // end for l
                    }
                }           // end for j
            
            }   // end if para CamionesS en arco
            
        }   // end if
    }       // end fr recorriendo arcos
    
}   // end fn salidaTransportesCabecera

//función para estadísticas de Bonnie
void salida_capacidades(ofstream& SalidaCapac, Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, vector<Producto>& Prod, vector<Familia>& FA)
{
    SalidaCapac << "idCamion;tipoCamion;tipoViaje;nodo;ruta;tiempoLlegada(segundos);tiempoSalida(segundos);separador;compartido;sector;estado;pallets;kilos;operacion;" << endl;
    SalidaCapac << setiosflags(ios_base::fixed);
    SalidaCapac << setprecision(0);
    
    for(unsigned int k=0; k<Nodes.size(); k++)
    {
 //       SalidaCapac << "Node: " << Nodes[k].get_cod() << endl;
        string ss = G.GetInOutCapacidades(k, Edges, Nodes, Prod, FA);
        SalidaCapac << ss;
    }
}       // end salida capacidades


struct duoSchedule
{
    string nid; time_t schedule;
    duoSchedule(string nd, time_t sch): nid(nd), schedule(sch) {}

    bool operator< (const duoSchedule &other) const {
        return schedule < other.schedule; }
};

int findEdge(string no, string nd, vector <Node>& Nodes, vector<Edge>& Edges)
{
    for(unsigned int i=0; i<Edges.size(); i++)
    {
        if((no == Nodes[Edges[i].v()].get_cod())&&(nd == Nodes[Edges[i].w()].get_cod()))
           return(i);
    }
           return(-1);
}

void salida_schedules(ofstream& SalidaSchedules, vector <Node>& Nodes, vector<Edge>& Edges, int lastCamionID)
{
    for(unsigned int i=0; i<lastCamionID; i++)
    {
        vector<duoSchedule> Times;
        Times.clear();
        SalidaSchedules << i+1 << ";";
        for(unsigned j=0; j<Nodes.size(); j++)
        {
            double TCD = -1;
            if(Nodes[j].get_type()==1)
                TCD = Nodes[j].getTcarga();
            else
                TCD = Nodes[j].getTdescarga();
            int pos;
            int sch = Nodes[j].findCamionSchedule(i+1, &pos);
            time_t tinicio = 0;
            time_t tfinal = 0;
            if(sch == 0)
                tinicio = Nodes[j].Schedule0[pos].programacion;
            else if(sch == 1)
                tinicio = Nodes[j].Schedule1[pos].programacion;
            else if(sch == 2)
                tinicio = Nodes[j].Schedule2[pos].programacion;
            else if(sch == 3)
                tinicio = Nodes[j].Schedule3[pos].programacion;
       
            tfinal = tinicio + TCD*3600;
            if(sch >=0)
            {
                SalidaSchedules << Nodes[j].get_cod() << ";" << tinicio << ";" << tfinal << ";;";
                Times.push_back(duoSchedule(Nodes[j].get_cod(),tinicio));
                Times.push_back(duoSchedule(Nodes[j].get_cod(),tfinal));
            }
        }
        sort(Times.begin(),Times.end());
        if(Times.size()==4)
        {
            time_t delta = Times[2].schedule - Times[1].schedule;
            SalidaSchedules << delta << ";;";
            int eid = findEdge(Times[1].nid, Times[2].nid, Nodes, Edges);
            if(eid >=0)
                SalidaSchedules << Edges[eid].getTViaje()*3600 << ";";
        }
        else if(Times.size()==6)
        {
            time_t delta1 = Times[2].schedule - Times[1].schedule;
            time_t delta2 = Times[4].schedule - Times[3].schedule;
            SalidaSchedules << delta1 << ";" << delta2 << ";;";
            int eid1 = findEdge(Times[1].nid, Times[2].nid, Nodes, Edges);
            int eid2 = findEdge(Times[3].nid, Times[4].nid, Nodes, Edges);
            if(eid1 >=0)
                SalidaSchedules << Edges[eid1].getTViaje()*3600 << ";";
            if(eid2 >=0)
                SalidaSchedules << Edges[eid2].getTViaje()*3600 << ";";
        }
        SalidaSchedules << endl;
    }
}

// FUNCIÓN QUE IMPRIME RESULTADOS DETALLADOS DE LA ASIGNACIÓN
void salida_detalle(ofstream& SalidaDetalle, Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, vector<Producto>& Prod)
{
    SalidaDetalle << "IdCamion;Clasificacion;IdPallet;Material;CodeNorigen;CodeNdestino;Ncajas;idNodo;" << endl;
    SalidaDetalle << setiosflags(ios_base::fixed);
    SalidaDetalle << setprecision(0);
    for(unsigned int i=0; i<Edges.size(); i++)
    {
        if(Edges[i].CamionesD.size()>0)
        {
            for(unsigned int j=0; j<Edges[i].CamionesD.size(); j++)
            {
                if(Edges[i].CamionesD[j].carga.size() > 0)
                {
                    for(unsigned int k=0; k<Edges[i].CamionesD[j].carga.size(); k++)
                    {
                        if(Edges[i].CamionesD[j].carga[k].BC.size() > 0)
                        {
                            for(unsigned int l=0; l<Edges[i].CamionesD[j].carga[k].BC.size(); l++)
                            {
                                if(Edges[i].CamionesD[j].carga[k].BC[l].getCajas() > 0)
                                {
                                    SalidaDetalle << Edges[i].CamionesD[j].get_idc() << ";" << Edges[i].CamionesD[j].ClasifCamion << ";";
                                    SalidaDetalle << Edges[i].CamionesD[j].carga[k].getid_pallet() << ";" << Edges[i].CamionesD[j].carga[k].BC[l].getProduct(Prod) << ";";
                                    SalidaDetalle << Nodes[Edges[i].v()].get_cod() << ";" << Nodes[Edges[i].w()].get_cod() << ";";
                                    if((Edges[i].CamionesD[j].carga[k].BC[l].getSuper())&&(Nodes[Edges[i].w()].get_type() == 2))
                                        SalidaDetalle << "S;";
                                    else
                                        SalidaDetalle << ";";
                                    string NDP = Nodes[Edges[i].w()].get_cod();
                                    if(Edges[i].CamionesD[j].carga[k].get_next_edge() != -1)
                                        NDP = Nodes[Edges[Edges[i].CamionesD[j].carga[k].get_next_edge()].w()].get_cod();
                                    
                                    SalidaDetalle << Edges[i].CamionesD[j].carga[k].BC[l].getCajas() << ";" << NDP << ";" << endl;
                                }
                             
                            }   // end for l
                        }
                        
                    }   // end for k
                }
                
            }   // end for j
        }       // end if CamionesD
        
        if(Edges[i].Camiones2.size()>0)
        {
            for(unsigned int j=0; j<Edges[i].Camiones2.size(); j++)
            {
                if(Edges[i].Camiones2[j].size() > 0)
                {
                    for(unsigned int p=0; p<Edges[i].Camiones2[j].size(); p++)
                    {
                        if(Edges[i].Camiones2[j][p].carga.size() > 0)
                        {
                            for(unsigned int k=0; k<Edges[i].Camiones2[j][p].carga.size(); k++)
                            {
                                if(Edges[i].Camiones2[j][p].carga[k].BC.size() > 0)
                                {
                                    for(unsigned int l=0; l<Edges[i].Camiones2[j][p].carga[k].BC.size(); l++)
                                    {
                                        if(Edges[i].Camiones2[j][p].carga[k].BC[l].getCajas() > 0)
                                        {
                                            SalidaDetalle << Edges[i].Camiones2[j][p].get_idc() << ";" << Edges[i].Camiones2[j][p].ClasifCamion << ";";
                                            SalidaDetalle << Edges[i].Camiones2[j][p].carga[k].getid_pallet() << ";" << Edges[i].Camiones2[j][p].carga[k].BC[l].getProduct(Prod) << ";";
                                            SalidaDetalle << Nodes[Edges[i].v()].get_cod() << ";" << Nodes[Edges[i].w()].get_cod() << ";";
                                            if((Edges[i].Camiones2[j][p].carga[k].BC[l].getSuper())&&(Nodes[Edges[i].w()].get_type() == 2))
                                                SalidaDetalle << "S;";
                                            else
                                                SalidaDetalle << ";";
                                            
                                            string NDP = Nodes[Edges[i].w()].get_cod();
                                            if(Edges[i].Camiones2[j][p].carga[k].get_next_edge() != -1)
                                                NDP = Nodes[Edges[Edges[i].Camiones2[j][p].carga[k].get_next_edge()].w()].get_cod();
                                            
                                            SalidaDetalle << Edges[i].Camiones2[j][p].carga[k].BC[l].getCajas() << ";" << NDP << ";" << endl;
                                        }
                                     
                                    }   // end for l
                                }
                                
                            }   // end for k
                        }       // end if carga size
                    }       // end for p
                    
                }       // end if Camiones2[j]
                
            }   // end for j

        }       // end if Camiones2
        
        if(Edges[i].CamionesS.size()>0)
        {
            for(unsigned int j=0; j<Edges[i].CamionesS.size(); j++)
            {
                if(Edges[i].CamionesS[j].size() > 0)
                {
                    for(unsigned int p=0; p<Edges[i].CamionesS[j].size(); p++)
                    {
                        if(Edges[i].CamionesS[j][p].carga.size() > 0)
                        {
                            for(unsigned int k=0; k<Edges[i].CamionesS[j][p].carga.size(); k++)
                            {
                                if(Edges[i].CamionesS[j][p].carga[k].BC.size() > 0)
                                {
                                    for(unsigned int l=0; l<Edges[i].CamionesS[j][p].carga[k].BC.size(); l++)
                                    {
                                        if(Edges[i].CamionesS[j][p].carga[k].BC[l].getCajas() > 0)
                                        {
                                            SalidaDetalle << Edges[i].CamionesS[j][p].get_idc() << ";" << Edges[i].CamionesS[j][p].ClasifCamion << ";";
                                            SalidaDetalle << Edges[i].CamionesS[j][p].carga[k].getid_pallet() << ";" << Edges[i].CamionesS[j][p].carga[k].BC[l].getProduct(Prod) << ";";
                                            SalidaDetalle << Nodes[Edges[i].v()].get_cod() << ";" << Nodes[Edges[i].w()].get_cod() << ";";
                                            if((Edges[i].CamionesS[j][p].carga[k].BC[l].getSuper())&&(Nodes[Edges[i].w()].get_type() == 2))
                                                SalidaDetalle << "S;";
                                            else
                                                SalidaDetalle << ";";
                                            
                                            string NDP = Nodes[Edges[i].w()].get_cod();
                                            if(Edges[i].CamionesS[j][p].carga[k].get_next_edge() != -1)
                                                NDP = Nodes[Edges[Edges[i].CamionesS[j][p].carga[k].get_next_edge()].w()].get_cod();
                                            
                                            SalidaDetalle << Edges[i].CamionesS[j][p].carga[k].BC[l].getCajas() << ";" << NDP << ";" << endl;
                                        }
                                     
                                    }   // end for l
                                }
                                
                            }   // end for k
                        }       // end if carga size
                    }       // end for p
                    
                }       // end if CamionesS[j]
                
            }   // end for j

        }       // end if CamionesS
        
    }   // end for i
    
}       // end salida_detalle

bool check_vector(vector<int>& pp)
{
    for(unsigned int k=0; k<pp.size(); k++)
    {
        if(pp[k] >= 1)
            return(true);
    }
    return(false);
}

string PrintMatriz(vector<vector<int>>& mat)
{
    stringstream s;
    for(unsigned int i=0; i<mat.size(); i++)
    {
        for(unsigned int j=0; j<mat[i].size(); j++)
        {
            s << mat[i][j] << ";";
        }
        s << endl;
    }
    return s.str();
}

//función que busca demanda
double Demanda(vector<Node>& Nodes, string CodeND, string CodProd, bool super)
{
    for(unsigned int i=0; i<Nodes.size(); i++)
    {
        for(unsigned int j=0; j<Nodes[i].demanda.size(); j++)
        {
            if((Nodes[i].demanda[j].Codigo == CodProd)&&(Nodes[i].get_cod() == CodeND)&&(Nodes[i].demanda[j].Supermercado==super))
                return(Nodes[i].demanda[j].amount);
        }
    }
    return(0);
}

struct trioCond
{
    string Nodo; string CodProd; string Cond;
    trioCond (string p1, string p2, string p3): Nodo(p1), CodProd(p2), Cond(p3) {}
};

void CheckCuad(vector<trioCond>& CuadOpcion, ifstream& inputa)
{
    vector<string> row;
    string line, topic;
    
    while (!inputa.eof())
    {
        row.clear();
        getline(inputa, line, '\n');

        if (!line.empty())
        {
            istringstream s(line);
            while (getline(s, topic, ';'))
                row.push_back(topic);
            
            CuadOpcion.push_back(trioCond(row[0],row[1],row[2]));
        }
    }
}       // end lectura archivo condatos SET

string PrintCuadCond(vector<trioCond>& CuadOpcion, string CodeNodo, string CondeProducto)
{
    for(unsigned int i=0; i<CuadOpcion.size(); i++)
    {
        if((CuadOpcion[i].CodProd == CondeProducto)&&(CuadOpcion[i].Nodo == CodeNodo))
            return(CuadOpcion[i].Cond);
    }
    return("NO ESTA");
}

//función que genera como output detalle2, con cajas por producto desde su origen
void salida_detalle2(ofstream& SalidaDetalle2, Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, vector<Producto>& Prod)
{
    set<string> Plantas = CreaSetPlantas(Nodes);
    set<string>::iterator it;
    SalidaDetalle2 << "TipoViaje;Material;CodeNdestino;Super;";
    for(it = Plantas.begin(); it != Plantas.end(); ++it)
        SalidaDetalle2 << *it << ";";
    SalidaDetalle2 << endl;
    SalidaDetalle2 << setiosflags(ios_base::fixed);
    SalidaDetalle2 << setprecision(0);
    
    for(unsigned int k=0; k<Prod.size(); k++)
    {
        for(unsigned int p=0; p<Nodes.size(); p++)
        {
            bool TV;
            vector<vector<int>> Cajas = G.IdentificaOrigen(Prod[k].get_code(), Nodes[p].get_cod(), Plantas, Edges, Nodes, Prod, &TV);
            string tipoV;
            if(TV)
                tipoV = "primario";
            else
                tipoV = "interplanta";
            
            if(check_vector(Cajas[0]))  //no es supermercado
            {
                SalidaDetalle2 << tipoV << ";" << Prod[k].get_code() << ";" << Nodes[p].get_cod() << ";;";
                for(unsigned i=0; i<Cajas[0].size(); i++)
                {
                    if(Cajas[0][i] >= 1)
                        SalidaDetalle2 << Cajas[0][i] << ";";
                    else
                        SalidaDetalle2 << ";";
                }
                SalidaDetalle2 << endl;
            }   // end if no S
            if(check_vector(Cajas[1]))  //si es supermercado
            {
                SalidaDetalle2 << tipoV << ";" << Prod[k].get_code() << ";" << Nodes[p].get_cod() << ";S;";
                for(unsigned i=0; i<Cajas[1].size(); i++)
                {
                    if(Cajas[1][i] >= 1)
                        SalidaDetalle2 << Cajas[1][i] << ";";
                    else
                        SalidaDetalle2 << ";";
                }
                SalidaDetalle2 << endl;
            }   //end if si S
            
        }   // end for recorriendo nodos
    }       // end for recorriendo productos
    
}   // end fn


// FUNCIÓN PARA VALIDAR INDICADORES (COMPARA COMPUTO AGREGADO CON COMPUTO A PARTIR DE LA ASIGNACION DE LA HEURÍSTICA

void check_indicadores(ofstream& SalidaCheck, Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, vector<Producto>& Prod)
{
    SalidaCheck << "Porcentaje de utilización de transporte en kilos: CHECK" << endl;
    SalidaCheck << endl;
    for (unsigned int k = 0; k < Edges.size(); k++)
    {
        if (Edges[k].fleet_size() > 0)
        {
            SalidaCheck << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << Edges[k].sumUsoX_camiones(Prod,"-1",1) << ";" << Edges[k].sumUsoX(Edges[k].x,Prod) + Edges[k].sumUsoX(Edges[k].x2,Prod) + Edges[k].sumUsoX(Edges[k].xr,Prod) + Edges[k].sumUsoX(Edges[k].xs,Prod) << ";" << Edges[k].sumYKilos_camiones(1) << ";" << Edges[k].sumYKilos(Edges[k].y) + Edges[k].sumYKilos(Edges[k].y2) + Edges[k].sumYKilos(Edges[k].ys) << ";" << endl;
        }
    }       // end for
    SalidaCheck << endl;
    SalidaCheck << "Porcentaje de utilización de transporte en Pallets: CHECK" << endl;
    SalidaCheck << endl;
    for (unsigned int k = 0; k < Edges.size(); k++)
    {
        if (Edges[k].fleet_size() > 0)
        {
            SalidaCheck << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << Edges[k].sumUsoP_camiones(Prod,"-1",1) << ";" << Edges[k].sumUsoP(Edges[k].x,Prod) + Edges[k].sumUsoP(Edges[k].x2,Prod) + Edges[k].sumUsoP(Edges[k].xr,Prod) + Edges[k].sumUsoP(Edges[k].xs,Prod) << ";" << Edges[k].sumYPallets_camiones(1) << ";" << Edges[k].sumYPallets(Edges[k].y) + Edges[k].sumYPallets(Edges[k].y2) + Edges[k].sumYPallets(Edges[k].ys) << ";" << endl;
        }
    }       // end for
    SalidaCheck << endl;
    
    SalidaCheck << "Porcentaje de utilización de transporte en kilos Consolidacion: CHECK" << endl;
    SalidaCheck << endl;
    for (unsigned int k = 0; k < Edges.size(); k++)
    {
        if ((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 1) && (Nodes[Edges[k].w()].get_type() == 1))
        {
            SalidaCheck << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << Edges[k].sumUsoX_camiones(Prod,"-1",2) << ";" <<  Edges[k].sumUsoX(Edges[k].xr,Prod) << ";" << Edges[k].sumYKilos_camiones(2) << ";" << Edges[k].sumYKilos(Edges[k].y) << ";" << endl;
        }
    }       // end for
    SalidaCheck << endl;
    SalidaCheck << "Porcentaje de utilización de transporte en Pallets Consolidación: CHECK" << endl;
    SalidaCheck << endl;
    for (unsigned int k = 0; k < Edges.size(); k++)
    {
        if ((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 1) && (Nodes[Edges[k].w()].get_type() == 1))
        {
            SalidaCheck << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << Edges[k].sumUsoP_camiones(Prod,"-1",2) << ";" << Edges[k].sumUsoP(Edges[k].xr,Prod) << ";" << Edges[k].sumYPallets_camiones(2) << ";" << Edges[k].sumYPallets(Edges[k].y) << ";" << endl;
        }
    }       // end for
    SalidaCheck << endl;
    
    SalidaCheck << "Porcentaje de utilización de transporte en kilos i-j-k: CHECK" << endl;
    SalidaCheck << endl;
    for (unsigned int k = 0; k < Edges.size(); k++)
    {
        if (Edges[k].fleet_size() > 0)
       // if ((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 1) && (Nodes[Edges[k].w()].get_type() == 1))
        {
            SalidaCheck << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << Edges[k].sumUsoX_camiones(Prod,"-1",3) << ";" <<  Edges[k].sumUsoX(Edges[k].x2,Prod) + Edges[k].sumUsoX(Edges[k].xs,Prod) << ";" << Edges[k].sumYKilos_camiones(3) << ";" << Edges[k].sumYKilos(Edges[k].y2) + Edges[k].sumYKilos(Edges[k].ys) << ";" << endl;
        }
    }       // end for
    SalidaCheck << endl;
    SalidaCheck << "Porcentaje de utilización de transporte en Pallets i-j-k: CHECK" << endl;
    SalidaCheck << endl;
    for (unsigned int k = 0; k < Edges.size(); k++)
    {
        if (Edges[k].fleet_size() > 0)
//        if ((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 1) && (Nodes[Edges[k].w()].get_type() == 1))
        {
            SalidaCheck << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << Edges[k].sumUsoP_camiones(Prod,"-1",3) << ";" << Edges[k].sumUsoP(Edges[k].x2,Prod) + Edges[k].sumUsoP(Edges[k].xs,Prod) << ";" << Edges[k].sumYPallets_camiones(3) << ";" << Edges[k].sumYPallets(Edges[k].y2) + Edges[k].sumYPallets(Edges[k].ys) << ";" << endl;
        }
    }       // end for
    SalidaCheck << endl;
    
}           // end check_indicadores

// FUNCIÓN QUE CALCULA LOS DISTINTOS INDICADORES A PARTIR DE LA CARGA DE LOS CAMIONES

void compute_indicadores(ofstream& SalidaModelo1, ofstream& SalidaModelo2, ofstream& SalidaModelo3, ofstream& SalidaModelo4, ofstream& SalidaModelo5, ofstream& SalidaModelo6, ofstream& SalidaModelo7, ofstream& SalidaModelo8, ofstream& SalidaModelo9, ofstream& SalidaModelo10, ofstream& SalidaModelo11, ofstream& SalidaModelo12, ofstream& SalidaModelo13, ofstream& SalidaModelo14, ofstream& SalidaModelo15, ofstream& SalidaModelo16, Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, vector<Producto>& Prod)
{
	// computamos Costos distribUcion Total +++++++++++++++++++++++++++++++++++++++++
 
	SalidaModelo1 << "CodigoNodoOrigen;CodigoNodoDestino;Cvariable; " << endl;
    SalidaModelo1 << setiosflags(ios_base::fixed);
    
    double CVTotal1 = 0;        // se computa indicador global 1
	for (unsigned int k = 0; k < Edges.size(); k++)
	{
		if ((Edges[k].fleet_size() > 0))
		{
			double CVariable = Edges[k].sumCosto(Prod,Nodes,1);
            CVTotal1 += CVariable;
            if((CVariable - floor(CVariable)) == 0)
                SalidaModelo1 << setprecision(0);
            else
                SalidaModelo1 << setprecision(NDEC);
			SalidaModelo1 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << CVariable << ";" << endl;
		}
	}       // end for
    
    // computamos Costos distribUcion interplanta +++++++++++++++++++++++++++++++++++++++++
    SalidaModelo2 << "CodigoNodoOrigen;CodigoNodoDestino;Cvariable; " << endl;
    SalidaModelo2 << setiosflags(ios_base::fixed);
    double CVTotal2 = 0;        // se computa indicador global 2
    for (unsigned int k = 0; k < Edges.size(); k++)
    {
        if ((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 1) && (Nodes[Edges[k].w()].get_type() == 1))
        {
            double CVariable = Edges[k].sumCosto(Prod,Nodes,2);
            CVTotal2 += CVariable;
            if((CVariable - floor(CVariable)) == 0)
                SalidaModelo2 << setprecision(0);
            else
                SalidaModelo2 << setprecision(NDEC);
            SalidaModelo2 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << CVariable << ";" << endl;
        }
    }       // end for

    // computamos Costos distribUcion transporte primario +++++++++++++++++++++++++++++++++++++++++
    SalidaModelo3 << "CodigoNodoOrigen;CodigoNodoDestino;Cvariable; " << endl;
    SalidaModelo3 << setiosflags(ios_base::fixed);
    double CVTotal3 = 0;        // se computa indicador global 3
    for (unsigned int k = 0; k < Edges.size(); k++)
    {
        if ((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 1) && (Nodes[Edges[k].w()].get_type() == 1))
        {
            double CVariable = Edges[k].sumCosto(Prod,Nodes,3);
            CVTotal3 += CVariable;
            if((CVariable - floor(CVariable)) == 0)
                SalidaModelo3 << setprecision(0);
            else
                SalidaModelo3 << setprecision(NDEC);
            SalidaModelo3 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << CVariable << ";" << endl;
        }
        if ((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 1) && (Nodes[Edges[k].w()].get_type() == 2))
        {
            double CVariable = Edges[k].sumCosto(Prod,Nodes,1);
            CVTotal3 += CVariable;
            double Dif = CVariable - floor(CVariable);
            if(Dif == 0)
                SalidaModelo3 << setprecision(0);
            else
                SalidaModelo3 << setprecision(NDEC);
            SalidaModelo3 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << CVariable << ";" << endl;
        }
        
        if ((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 2) && (Nodes[Edges[k].w()].get_type() == 2))
        {
            double CVariable = Edges[k].sumCosto(Prod,Nodes,1);
            CVTotal3 += CVariable;
            double Dif = CVariable - floor(CVariable);
            if(Dif == 0)
                SalidaModelo3 << setprecision(0);
            else
                SalidaModelo3 << setprecision(NDEC);
            SalidaModelo3 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << CVariable << ";" << endl;
        }
        
    }       // end for

	// computamos Porcentaje de utilización en kilos +++++++++++++++++++++++++++++++++++++++++++++++++++++
	SalidaModelo4 << "CodigoNodoOrigen;CodigoNodoDestino;UsoKilos;CapacidadKilos;UsoKilos/CapacidadKilos;" << endl;
    SalidaModelo4 << setiosflags(ios_base::fixed);
    
    double SumXT = 0;
    double SumYT = 0;
	for (unsigned int k = 0; k < Edges.size(); k++)
	{
		if (Edges[k].fleet_size() > 0)
		{
            SumXT += Edges[k].sumUsoX_camiones(Prod,"-1",1);
            SalidaModelo4 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << setprecision(NDEC2) << Edges[k].sumUsoX_camiones(Prod,"-1",1) << ";" << setprecision(0) << Edges[k].sumYKilos_camiones(1) << ";";
			if (Edges[k].sumYKilos_camiones(1) > 0)
            {
                SumYT += Edges[k].sumYKilos_camiones(1);
				SalidaModelo4 << setprecision(NDEC) << (double)Edges[k].sumUsoX_camiones(Prod,"-1",1) / (double)Edges[k].sumYKilos_camiones(1) << ";";
            }
			else
				SalidaModelo4 << "--;";
			SalidaModelo4 << endl;
		}
	}       // end for
    
    double IndicadorGlobal4 = (double) SumXT / (double) SumYT ;

    // computamos Porcentaje de utilización en kilos interplanta +++++++++++++++++++++++++++++++++++++++++++++++++++++
    SalidaModelo5 << "CodigoNodoOrigen;CodigoNodoDestino;UsoKilos;CapacidadKilos;UsoKilos/CapacidadKilos;" << endl;
    SalidaModelo5 << setiosflags(ios_base::fixed);
    
    double SumXI = 0;
    double SumYI = 0;
    for (unsigned int k = 0; k < Edges.size(); k++)
    {
        if((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 1) && (Nodes[Edges[k].w()].get_type() == 1))
        {
            SumXI += Edges[k].sumUsoX_camiones(Prod,"-1",2);
            SalidaModelo5 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << setprecision(NDEC2) << Edges[k].sumUsoX_camiones(Prod,"-1",2) << ";" << setprecision(0) << Edges[k].sumYKilos_camiones(2) << ";";
            if (Edges[k].sumYKilos_camiones(2) > 0)
            {
                SumYI += Edges[k].sumYKilos_camiones(2);
                SalidaModelo5 << setprecision(NDEC) << (double)Edges[k].sumUsoX_camiones(Prod,"-1",2) / (double)Edges[k].sumYKilos_camiones(2) << ";";
            }
            else
                SalidaModelo5 << "--;";
            SalidaModelo5 << endl;
        }
    }       // end for
 
    double IndicadorGlobal5 = (double) SumXI / (double) SumYI ;
    
    // computamos Porcentaje de utilización en kilos primario +++++++++++++++++++++++++++++++++++++++++++++++++++++
    SalidaModelo6 << "CodigoNodoOrigen;CodigoNodoDestino;UsoKilos;CapacidadKilos;UsoKilos/CapacidadKilos;" << endl;
    SalidaModelo6 << setiosflags(ios_base::fixed);
    
    double SumXp = 0;
    double SumYp = 0;
    for (unsigned int k = 0; k < Edges.size(); k++)
    {
        if((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 1) && (Nodes[Edges[k].w()].get_type() == 1))
        {
            SumXp += Edges[k].sumUsoX_camiones(Prod,"-1",3);
            SalidaModelo6 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << setprecision(NDEC2) << Edges[k].sumUsoX_camiones(Prod,"-1",3) << ";" << setprecision(0) << Edges[k].sumYKilos_camiones(3) << ";";
            if (Edges[k].sumYKilos_camiones(3) > 0)
            {
                SumYp += Edges[k].sumYKilos_camiones(3);
                SalidaModelo6 << setprecision(NDEC) << (double)Edges[k].sumUsoX_camiones(Prod,"-1",3) / (double)Edges[k].sumYKilos_camiones(3) << ";";
            }
            else
                SalidaModelo6 << "--;";
            SalidaModelo6 << endl;
        }
        if((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 1) && (Nodes[Edges[k].w()].get_type() == 2))
        {
            SumXp += Edges[k].sumUsoX_camiones(Prod,"-1",1);
            SalidaModelo6 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << setprecision(NDEC2) << Edges[k].sumUsoX_camiones(Prod,"-1",1) << ";" << setprecision(0) << Edges[k].sumYKilos_camiones(1) << ";";
            if (Edges[k].sumYKilos_camiones(1) > 0)
            {
                SumYp += Edges[k].sumYKilos_camiones(1);
                SalidaModelo6 << setprecision(NDEC) << (double)Edges[k].sumUsoX_camiones(Prod,"-1",1) / (double)Edges[k].sumYKilos_camiones(1) << ";";
            }
            else
                SalidaModelo6 << "--;";
            SalidaModelo6 << endl;
        }
        if((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 2) && (Nodes[Edges[k].w()].get_type() == 2))
        {
            SumXp += Edges[k].sumUsoX_camiones(Prod,"-1",1);
            SalidaModelo6 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << setprecision(NDEC2) << Edges[k].sumUsoX_camiones(Prod,"-1",1) << ";" << setprecision(0) << Edges[k].sumYKilos_camiones(1) << ";";
            if (Edges[k].sumYKilos_camiones(1) > 0)
            {
                SumYp += Edges[k].sumYKilos_camiones(1);
                SalidaModelo6 << setprecision(NDEC) << (double)Edges[k].sumUsoX_camiones(Prod,"-1",1) / (double)Edges[k].sumYKilos_camiones(1) << ";";
            }
            else
                SalidaModelo6 << "--;";
            SalidaModelo6 << endl;
        }
        
    }       // end for

    double IndicadorGlobal6 = (double) SumXp / (double) SumYp ;
    
	// computamos Porcentaje de utilización en pallets +++++++++++++++++++++++++++++++++++++++++++++++++++++
	SalidaModelo7 << "CodigoNodoOrigen;CodigoNodoDestino;UsoPallets;CapacidadPallets;UsoPallets/CapacidadPallets;" << endl;
    SalidaModelo7 << setiosflags(ios_base::fixed);

    double SumPT = 0;
    double SumYPT = 0;
	for (unsigned int k = 0; k < Edges.size(); k++)
	{
		if (Edges[k].fleet_size() > 0)
		{
            SumPT += Edges[k].sumUsoP_camiones(Prod,"-1",1);
            if(Edges[k].sumUsoP_camiones(Prod,"-1",1) - floor(Edges[k].sumUsoP_camiones(Prod,"-1",1) > 0))
                SalidaModelo7 << setprecision(NDEC2);
            else
                SalidaModelo7 << setprecision(0);
			SalidaModelo7 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << Edges[k].sumUsoP_camiones(Prod,"-1",1) << ";" << setprecision(0) << Edges[k].sumYPallets_camiones(1) << ";";
			if (Edges[k].sumYPallets_camiones(1)  > 0)
            {
                SumYPT += Edges[k].sumYPallets_camiones(1);
				SalidaModelo7 << setprecision(NDEC) << (double)Edges[k].sumUsoP_camiones(Prod,"-1",1) / (double)Edges[k].sumYPallets_camiones(1) << ";";
            }
			else
				SalidaModelo7 << "--;";
			SalidaModelo7 << endl;
		}
	}       // end for

    double IndicadorGlobal7 = (double) SumPT / (double) SumYPT ;
    
    // computamos Porcentaje de utilización en pallets interplanta +++++++++++++++++++++++++++++++++++++++++++++++++++++
    SalidaModelo8 << "CodigoNodoOrigen;CodigoNodoDestino;UsoPallets;CapacidadPallets;UsoPallets/CapacidadPallets;" << endl;
    SalidaModelo8 << setiosflags(ios_base::fixed);

    double SumPI = 0;
    double SumYPI = 0;
    for (unsigned int k = 0; k < Edges.size(); k++)
    {
        if((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 1) && (Nodes[Edges[k].w()].get_type() == 1))
        {
            SumPI += Edges[k].sumUsoP_camiones(Prod,"-1",2);
            if(Edges[k].sumUsoP_camiones(Prod,"-1",2) - floor(Edges[k].sumUsoP_camiones(Prod,"-1",2)))
                SalidaModelo8 << setprecision(NDEC2);
             else
                 SalidaModelo8 << setprecision(0);
            SalidaModelo8 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << Edges[k].sumUsoP_camiones(Prod,"-1",2) << ";" << setprecision(0) << Edges[k].sumYPallets_camiones(2) << ";";
            if (Edges[k].sumYPallets_camiones(2)  > 0)
            {
                SumYPI += Edges[k].sumYPallets_camiones(2);
                SalidaModelo8 << setprecision(NDEC) << (double)Edges[k].sumUsoP_camiones(Prod,"-1",2) / (double)Edges[k].sumYPallets_camiones(2) << ";";
            }
            else
                SalidaModelo8 << "--;";
            SalidaModelo8 << endl;
        }
    }       // end for
    
    double IndicadorGlobal8 = (double) SumPI / (double) SumYPI ;
    
    // computamos Porcentaje de utilización en pallets primario +++++++++++++++++++++++++++++++++++++++++++++++++++++
    SalidaModelo9 << "CodigoNodoOrigen;CodigoNodoDestino;UsoPallets;CapacidadPallets;UsoPallets/CapacidadPallets;" << endl;
    SalidaModelo9 << setiosflags(ios_base::fixed);

    double SumPp = 0;
    double SumYPp = 0;
    for (unsigned int k = 0; k < Edges.size(); k++)
    {
        if((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 1) && (Nodes[Edges[k].w()].get_type() == 1))
        {
            SumPp += Edges[k].sumUsoP_camiones(Prod,"-1",3);
            if(Edges[k].sumUsoP_camiones(Prod,"-1",3) - floor(Edges[k].sumUsoP_camiones(Prod,"-1",3)))
                SalidaModelo9 << setprecision(NDEC2);
             else
                 SalidaModelo9 << setprecision(0);
            SalidaModelo9 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << Edges[k].sumUsoP_camiones(Prod,"-1",3) << ";" << setprecision(0) << Edges[k].sumYPallets_camiones(3) << ";";
            if (Edges[k].sumYPallets_camiones(3)  > 0)
            {
                SumYPp += Edges[k].sumYPallets_camiones(3);
                SalidaModelo9 << setprecision(NDEC) << (double)Edges[k].sumUsoP_camiones(Prod,"-1",3) / (double)Edges[k].sumYPallets_camiones(3) << ";";
            }
            else
                SalidaModelo9 << "--;";
            SalidaModelo9 << endl;
        }
        if((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 1) && (Nodes[Edges[k].w()].get_type() == 2))
        {
            SumPp += Edges[k].sumUsoP_camiones(Prod,"-1",1);
            if(Edges[k].sumUsoP_camiones(Prod,"-1",1) - floor(Edges[k].sumUsoP_camiones(Prod,"-1",1)))
                SalidaModelo9 << setprecision(NDEC2);
             else
                 SalidaModelo9 << setprecision(0);
            SalidaModelo9 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << Edges[k].sumUsoP_camiones(Prod,"-1",1) << ";" << setprecision(0) << Edges[k].sumYPallets_camiones(1) << ";";
            if (Edges[k].sumYPallets_camiones(1)  > 0)
            {
                SumYPp += Edges[k].sumYPallets_camiones(1);
                SalidaModelo9 << setprecision(NDEC) << (double)Edges[k].sumUsoP_camiones(Prod,"-1",1) / (double)Edges[k].sumYPallets_camiones(1) << ";";
            }
            else
                SalidaModelo9 << "--;";
            SalidaModelo9 << endl;
        }
        if((Edges[k].fleet_size() > 0) && (Nodes[Edges[k].v()].get_type() == 2) && (Nodes[Edges[k].w()].get_type() == 2))
        {
            SumPp += Edges[k].sumUsoP_camiones(Prod,"-1",1);
            if(Edges[k].sumUsoP_camiones(Prod,"-1",1) - floor(Edges[k].sumUsoP_camiones(Prod,"-1",1)))
                SalidaModelo9 << setprecision(NDEC2);
             else
                 SalidaModelo9 << setprecision(0);
            SalidaModelo9 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << Edges[k].sumUsoP_camiones(Prod,"-1",1) << ";" << setprecision(0) << Edges[k].sumYPallets_camiones(1) << ";";
            if (Edges[k].sumYPallets_camiones(1)  > 0)
            {
                SumYPp += Edges[k].sumYPallets_camiones(1);
                SalidaModelo9 << setprecision(NDEC) << (double)Edges[k].sumUsoP_camiones(Prod,"-1",1) / (double)Edges[k].sumYPallets_camiones(1) << ";";
            }
            else
                SalidaModelo9 << "--;";
            SalidaModelo9 << endl;
        }
    }       // end for
    
    double IndicadorGlobal9 = (double) SumPp / (double) SumYPp ;

 // computamos Nivel de servicio +++++++++++++++++++++++++++++++++++++++++++++++++++++
	SalidaModelo10 << "CodigoNodo;;IDProducto;INCajas;DemandaCajas;INCajas/DemandaCajas;" << endl;
    SalidaModelo10 << setiosflags(ios_base::fixed);
    SalidaModelo11 << "CodigoNodo;INCajas;DemandaCajas;INCajas/DemandaCajas;" << endl;
    SalidaModelo11 << setiosflags(ios_base::fixed);
    
    double INF_ACUM_TOT = 0;
    double DEM_ACUM_TOT = 0;
    
	for (unsigned int k = 0; k < Nodes.size(); k++)
	{
		if (Nodes[k].get_type() == 2)    //se trata de sucursal
		{
            SalidaModelo10 << setprecision(0);
			double cumpl = 0;
			int count = 0;
            double INF_ACUM = 0;
            double DEM_ACUM = 0;
			if (Nodes[k].demanda.size() > 0)
			{
				for (unsigned int l = 0; l<Nodes[k].demanda.size(); l++)
				{
					double OUTF, INF;
                    
                    bool existeSuper = Nodes[k].demanda[l].Supermercado;
                    string Ncodigo = Nodes[k].get_cod();
                    if(existeSuper)
                        Ncodigo += ";CCS";
                    else
                        Ncodigo += ";";
					SalidaModelo10 << Ncodigo << ";" << Nodes[k].demanda[l].Codigo << ";";
					G.InOutXFiltrado(k, Nodes[k].demanda[l].Codigo, Edges, Nodes, Prod, 1, &OUTF, &INF,  existeSuper);
                    SalidaModelo10 << INF << ";" << Nodes[k].demanda[l].amount << ";";
                    if(Nodes[k].demanda[l].amount > 0)
                        SalidaModelo10 << setprecision(NDEC) << (double)INF / (double)Nodes[k].demanda[l].amount << ";" << endl;
                    else
                        SalidaModelo10 << "--;" << endl;

					if (Nodes[k].demanda[l].amount > 0)
					{
						cumpl += (double)INF / (double)Nodes[k].demanda[l].amount;
                        INF_ACUM += INF;
                        DEM_ACUM += Nodes[k].demanda[l].amount;
						count++;
					}
				}       // end for
			}           // end if

            //para computar un gran indicador global
            INF_ACUM_TOT += INF_ACUM;
            DEM_ACUM_TOT += DEM_ACUM;
            
            
            SalidaModelo11 << setprecision(0) << Nodes[k].get_cod() << ";" << INF_ACUM << ";" << DEM_ACUM << ";";
            if(DEM_ACUM > 0)
                SalidaModelo11 << setprecision(NDEC) << (double) INF_ACUM / (double) DEM_ACUM << ";" << endl;
            else
                SalidaModelo11 << "--;" << endl;
            
		}
	}       // end for
    
    double IndicadorGlobal10 = (double) INF_ACUM_TOT / (double) DEM_ACUM_TOT;
    
	// computamos Ratio $/kg ++++++++++++++++++++++++++++++++++++++++++++++++

	SalidaModelo12 << "CodigoOrigen;CodigoDestino;CVar;Tkilos;CVar/Tkilos;" << endl;
    SalidaModelo12 << setiosflags(ios_base::fixed);

	double CVcum = 0;
	double Tkilos = 0;

	for (unsigned int k = 0; k < Edges.size(); k++)
	{
		if (Edges[k].fleet_size() > 0)
		{
            double CVar = Edges[k].sumCosto(Prod, Nodes, 1);
            CVcum += CVar;
            double TK = Edges[k].sumUsoX_camiones(Prod,"-1",1);
            Tkilos += TK;
            
            SalidaModelo12 << Nodes[Edges[k].v()].get_cod() << ";" << Nodes[Edges[k].w()].get_cod() << ";" << setprecision(0) << CVar << ";" << setprecision(NDEC) << TK << ";" << setprecision(NDEC) << (double)(CVar) / (double)(TK) << ";" << endl;
            
		}
	}       // end for

    //computando indicador global (11)
    double IndGlobalRatioCK11 = (double)(CVcum) / (double)(Tkilos);
    
    
	// computamos Visita Nodos Pickup +++++++++++++++++++++++++++++++++++++++++++++++++++++
	SalidaModelo13 << "IDcamion;Clasificación;Ruta;Planta1;PLanta2;#PtosCarga;" << endl;
    SalidaModelo13 << setiosflags(ios_base::fixed);
    SalidaModelo13 << setprecision(0);
    
    int NRutas = 0;
    int NParadas = 0;
    
    for(unsigned int i=0; i<Edges.size(); i++)
    {
        if((Nodes[Edges[i].v()].get_type() == 1) && (Nodes[Edges[i].w()].get_type() == 1))
        {
            if(Edges[i].CamionesD.size()>0)
            {
                string ruta = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                for(unsigned int j=0; j<Edges[i].CamionesD.size(); j++)
                {
                    SalidaModelo13 << Edges[i].CamionesD[j].get_idc() << ";" << Edges[i].CamionesD[j].ClasifCamion << ";" << ruta << ";" << Nodes[Edges[i].v()].get_cod() << ";;" << 1 << ";" << endl;
                    NRutas++;
                    NParadas++;
                            
                }           // end for j
            }       // end if CamionesD
            
            // caso de Camiones2 (planta a planta) ++++++++++++++++++++++++++++++++++
            if(Edges[i].Camiones2.size() > 0)
            {
                string ruta = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                for(unsigned int j=0; j<Edges[i].Camiones2.size(); j++)
                {
                    if(Edges[i].Camiones2[j].size()>0)
                    {
                        for(unsigned int l=0; l<Edges[i].Camiones2[j].size(); l++)
                        {
                            NRutas++;
                            int next_link_id = Edges[i].Camiones2[j][l].get_nxt();
                            string ruta_final = ruta + "-" + Nodes[Edges[next_link_id].w()].get_cod();
                            bool doble_carga = Camion2(Edges[i].Camiones2[j][l],Edges);
                            if(!doble_carga)
                            {
                                SalidaModelo13 << Edges[i].Camiones2[j][l].get_idc() << ";" << Edges[i].Camiones2[j][l].ClasifCamion << ";" << ruta_final << ";" << Nodes[Edges[i].v()].get_cod() << ";;" << 1 << ";" << endl;
                                NParadas++;
                            }
                            else
                            {
                                SalidaModelo13 << Edges[i].Camiones2[j][l].get_idc() << ";" << Edges[i].Camiones2[j][l].ClasifCamion << ";" << ruta_final << ";" << Nodes[Edges[i].v()].get_cod() << ";" << Nodes[Edges[i].w()].get_cod() << ";" << 2 << ";" << endl;
                                NParadas = NParadas + 2;
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
                string ruta = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                for(unsigned int j=0; j<Edges[i].CamionesD.size(); j++)
                {
                    NRutas++;
                    NParadas++;
                    SalidaModelo13 << Edges[i].CamionesD[j].get_idc() << ";" << Edges[i].CamionesD[j].ClasifCamion << ";" << ruta << ";" << Nodes[Edges[i].v()].get_cod() << ";;" << 1 << ";" << endl;
                            
                }           // end for j
            }       // end if CamionesD
            
            // caso de CamionesS (planta a sucursal) ++++++++++++++++++++++++++++++++++
            if(Edges[i].CamionesS.size() > 0)
            {
                string ruta = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                for(unsigned int j=0; j<Edges[i].CamionesS.size(); j++)
                {
                    if(Edges[i].CamionesS[j].size()>0)
                    {
                        for(unsigned int l=0; l<Edges[i].CamionesS[j].size(); l++)
                        {
                            NRutas++;
                            int next_link_id = Edges[i].CamionesS[j][l].get_nxt();
                            string ruta_final = ruta + "-" + Nodes[Edges[next_link_id].w()].get_cod();
                            
                            SalidaModelo13 << Edges[i].CamionesS[j][l].get_idc() << ";" << Edges[i].CamionesS[j][l].ClasifCamion << ";" << ruta_final << ";" << Nodes[Edges[i].v()].get_cod() << ";;" << 1 << ";" << endl;
                                NParadas++;
                      
                        }       // end for l
                    }
                }           // end for j
            
            }   // end if para CamionesS en arco
            
        }
    }

    double IndicadorGlobal12 = (double)(NParadas) / (double)(NRutas);

	// computa Carga Nodos pickup ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	SalidaModelo14 << "CodigoPlanta;OUTKilos;INKilos;" << endl;
    SalidaModelo14 << setiosflags(ios_base::fixed);
    
    double IN_ACUM = 0;
    double OUT_ACUM = 0;
    
	for (unsigned int k = 0; k < Nodes.size(); k++)
	{
		if (Nodes[k].get_type() == 1)    //se trata de una planta
		{
			double INF, OUTF;
			G.InOutXTotalKilos(k, Edges, Nodes, Prod, 1 ,&OUTF, &INF);
            IN_ACUM += INF;
            OUT_ACUM += OUTF;
            
            if((OUTF-floor(OUTF)) > 0)
                SalidaModelo14 << setprecision(NDEC);
            else
                SalidaModelo14 << setprecision(0);
            SalidaModelo14 << Nodes[k].get_cod() << ";" << OUTF << ";";
            if((INF-floor(INF)) > 0)
                SalidaModelo14 << setprecision(NDEC);
            else
                SalidaModelo14 << setprecision(0);
            SalidaModelo14 << INF << ";" << endl;
		}   // end if
	}       // end for
    
    double IndicadorGlobal13_1 = OUT_ACUM;
    double IndicadorGlobal13_2 = IN_ACUM;

	// computa Camiones por planta ++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	SalidaModelo15 << "CodigoPLanta;OUTCamiones;INCamiones;" << endl;
    SalidaModelo15 << setiosflags(ios_base::fixed);
    SalidaModelo15 << setprecision(0);
    
    double IN_CAM = 0;
    double OUT_CAM = 0;
	for (unsigned int k = 0; k < Nodes.size(); k++)
	{
		if (Nodes[k].get_type() == 1)    //se trata de una planta
		{
			double INF, OUTF;
			G.InOut(k, "y", Edges, Nodes, Prod, 1 ,&OUTF, &INF);     //poner false en super da lo mismo acá
            IN_CAM += INF;
            OUT_CAM += OUTF;
			SalidaModelo15 << Nodes[k].get_cod() << ";" << OUTF << ";" << INF << ";" << endl;
		}   // end if
	}       // end for

    double IndicadorGlobal14_1 = OUT_CAM;
    double IndicadorGlobal14_2 = IN_CAM;
    
    // computa indicadore globales ++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    SalidaModelo16 << "Indicadores globales;;;;" << endl;
    SalidaModelo16 << ";;;;" << endl;
    SalidaModelo16 << setiosflags(ios_base::fixed);
    SalidaModelo16 << "Costos Distribución Total;" << setprecision(0) << CVTotal1 << ";;($);" << endl;
    SalidaModelo16 << "Costos Distribución Interplanta;" << CVTotal2 << ";;($);" << endl;
    SalidaModelo16 << "Costos Distribución Primario;" << CVTotal3 << ";;($);" << endl;
    SalidaModelo16 << "Porcentaje de utilización de transporte en kilos total;" << setprecision(NDEC) << IndicadorGlobal4 << ";;(%);" << endl;
    SalidaModelo16 << "Porcentaje de utilización de transporte en kilos Interplanta;" << IndicadorGlobal5 << ";;(%);" << endl;
    SalidaModelo16 << "Porcentaje de utilización de transporte en kilos primario;" << IndicadorGlobal6 << ";;(%);" << endl;
    SalidaModelo16 << "Porcentaje de utilización de transporte en pallets total;" << IndicadorGlobal7 << ";;(%);" << endl;
    SalidaModelo16 << "Porcentaje de utilización de transporte en pallets Interplanta;" << IndicadorGlobal8 << ";;(%);" << endl;
    SalidaModelo16 << "Porcentaje de utilización de transporte en pallets primario;" << IndicadorGlobal9 << ";;(%);" << endl;
    SalidaModelo16 << "Cumplimiento de la demanda;" << IndicadorGlobal10 << ";;(%);" << endl;
    SalidaModelo16 << "Ratio Costo/Kilo;" << IndGlobalRatioCK11 << ";;($/Kilo);" << endl;
    SalidaModelo16 << "VisitaNodosPlanta;" << IndicadorGlobal12 << ";;(Paradas/Ruta);" << endl;
    SalidaModelo16 << "CargaNodosPlanta(OUT-IN);" << setprecision(0) << IndicadorGlobal13_1 << ";" << IndicadorGlobal13_2 << ";(Kilos);" << endl;
    SalidaModelo16 << "CamionesNodosPlanta(OUT-IN);" << IndicadorGlobal14_1 << ";" << IndicadorGlobal14_2 << ";(Camiones);" << endl;

}   // end compute indicadores

bool LlevaDummy(Camion CAux)
{
    for(unsigned int i=0; i<CAux.carga.size(); i++)
    {
        if(CAux.carga[i].getid_pallet() == -100)
            return(true);
    }
    return(false);
}

void DetalleCargaCamiones(ofstream& SalidaDetalle, Graph& G, vector<Edge>& Edges, vector <Node>& Nodes, vector<Producto>& Prod, vector<Familia>& FA)
{
    // computamos Visita Nodos Pickup +++++++++++++++++++++++++++++++++++++++++++++++++++++
    SalidaDetalle << "IDcamion;Tipo;Clasificacion;Ruta;TTotal;P1C;P1F;P2C;P2F;#PtosCarga;Dummy;" << endl;
    SalidaDetalle << setiosflags(ios_base::fixed);
    SalidaDetalle << setprecision(0);
    
    for(unsigned int i=0; i<Edges.size(); i++)
    {
        if((Nodes[Edges[i].v()].get_type() == 1) && (Nodes[Edges[i].w()].get_type() == 1))
        {
            if(Edges[i].CamionesD.size()>0)
            {
                string ruta = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                for(unsigned int j=0; j<Edges[i].CamionesD.size(); j++)
                {
                    bool DUM = LlevaDummy(Edges[i].CamionesD[j]);
                    int nPF = Edges[i].CamionesD[j].nPallets() - Edges[i].CamionesD[j].nPalletsC(FA);
                    SalidaDetalle << Edges[i].CamionesD[j].get_idc() << ";" << Edges[i].CamionesD[j].get_stype() << ";" << Edges[i].CamionesD[j].ClasifCamion << ";" << ruta << ";" << Edges[i].CamionesD[j].TTotal << ";" << Edges[i].CamionesD[j].nPalletsC(FA) << ";" << nPF << ";;;" << 1 << ";" << DUM << ";" << endl;
                            
                }           // end for j
            }       // end if CamionesD
            
            // caso de Camiones2 (planta a planta) ++++++++++++++++++++++++++++++++++
            if(Edges[i].Camiones2.size() > 0)
            {
                string ruta = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                for(unsigned int j=0; j<Edges[i].Camiones2.size(); j++)
                {
                    if(Edges[i].Camiones2[j].size()>0)
                    {
                        for(unsigned int l=0; l<Edges[i].Camiones2[j].size(); l++)
                        {
                            bool DUM = LlevaDummy(Edges[i].Camiones2[j][l]);
                            int next_link_id = Edges[i].Camiones2[j][l].get_nxt();
                            string ruta_final = ruta + "-" + Nodes[Edges[next_link_id].w()].get_cod();
                            bool doble_carga = Camion2(Edges[i].Camiones2[j][l],Edges);
                            if(!doble_carga)
                            {
                                int nPF = Edges[i].Camiones2[j][l].nPallets() - Edges[i].Camiones2[j][l].nPalletsC(FA);
                                SalidaDetalle << Edges[i].Camiones2[j][l].get_idc() << ";" << Edges[i].Camiones2[j][l].get_stype() << ";" << Edges[i].Camiones2[j][l].ClasifCamion << ";" << ruta_final << ";" << Edges[i].Camiones2[j][l].TTotal << ";" << Edges[i].Camiones2[j][l].nPalletsC(FA) << ";" << nPF << ";" << Edges[i].Camiones2[j][l].nPalletsC(FA) << ";" << nPF << ";" << 1 << ";" << DUM << ";" << endl;
                            }
                            else
                            {
                                int ii,jj;
                                int second_id = Edges[i].Camiones2[j][l].get_nxt();
                                Edges[second_id].searchCamion(Edges[i].Camiones2[j][l].get_idc(), &ii, &jj);
                                int nPC = Edges[second_id].Camiones2[ii][jj].nPalletsC(FA);
                                int nPF = Edges[second_id].Camiones2[ii][jj].nPallets() - Edges[second_id].Camiones2[ii][jj].nPalletsC(FA);
                                
                                int nPF1 = Edges[i].Camiones2[j][l].nPallets() - Edges[i].Camiones2[j][l].nPalletsC(FA);
                                SalidaDetalle << Edges[i].Camiones2[j][l].get_idc() << ";" << Edges[i].Camiones2[j][l].get_stype() << ";" << Edges[i].Camiones2[j][l].ClasifCamion << ";" << ruta_final << ";" << Edges[i].Camiones2[j][l].TTotal << ";" << Edges[i].Camiones2[j][l].nPalletsC(FA) << ";" << nPF1 << ";" << nPC << ";" << nPF << ";" << 2 << ";" << DUM << ";" << endl;
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
                string ruta = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                for(unsigned int j=0; j<Edges[i].CamionesD.size(); j++)
                {
                    bool DUM = LlevaDummy(Edges[i].CamionesD[j]);
                    int nPF = Edges[i].CamionesD[j].nPallets() - Edges[i].CamionesD[j].nPalletsC(FA);
                    SalidaDetalle << Edges[i].CamionesD[j].get_idc() << ";" << Edges[i].CamionesD[j].get_stype() << ";" << Edges[i].CamionesD[j].ClasifCamion << ";" << ruta << ";" << Edges[i].CamionesD[j].TTotal << ";" <<  Edges[i].CamionesD[j].nPalletsC(FA) << ";" << nPF << ";;;" << 1 << ";" << DUM << ";" << endl;
                            
                }           // end for j
            }       // end if CamionesD
            
            // caso de CamionesS (planta a sucursal) ++++++++++++++++++++++++++++++++++
            if(Edges[i].CamionesS.size() > 0)
            {
                string ruta = Nodes[Edges[i].v()].get_cod() + "-" + Nodes[Edges[i].w()].get_cod();
                for(unsigned int j=0; j<Edges[i].CamionesS.size(); j++)
                {
                    if(Edges[i].CamionesS[j].size()>0)
                    {
                        for(unsigned int l=0; l<Edges[i].CamionesS[j].size(); l++)
                        {
                            bool DUM = LlevaDummy(Edges[i].CamionesS[j][l]);
                            int next_link_id = Edges[i].CamionesS[j][l].get_nxt();
                            string ruta_final = ruta + "-" + Nodes[Edges[next_link_id].w()].get_cod();
                            
                            int ii,jj;
                            int second_id = Edges[i].CamionesS[j][l].get_nxt();
                            Edges[second_id].searchCamionS(Edges[i].CamionesS[j][l].get_idc(), &ii, &jj);
                            int nPC = Edges[second_id].CamionesS[ii][jj].nPalletsC(FA);
                            int nPF = Edges[second_id].CamionesS[ii][jj].nPallets() - Edges[second_id].CamionesS[ii][jj].nPalletsC(FA);
                                
                            int nPF1 = Edges[i].CamionesS[j][l].nPallets() - Edges[i].CamionesS[j][l].nPalletsC(FA);
                            SalidaDetalle << Edges[i].CamionesS[j][l].get_idc() << ";" << Edges[i].CamionesS[j][l].get_stype() << ";" << Edges[i].CamionesS[j][l].ClasifCamion << ";" << ruta_final << ";" << Edges[i].CamionesS[j][l].TTotal << ";" << Edges[i].CamionesS[j][l].nPalletsC(FA) << ";" << nPF1 << ";" << nPC << ";" << nPF << ";" << 1 << ";" << DUM << ";" << endl;
                        
                        }       // end for l
                    }
                }           // end for j
            
            }   // end if para CamionesS en arco Planta a sucursal
        }
    }       // end for recorriendo edges

}   // end fn que muestra Pallets congelados y fresco de cada camión



int main(int argc, char *argv[])
{
    
	vector<Edge> e1;
	vector<Node> n1;
	vector<Producto> p1;
	vector<Familia> Fa;
	Graph g1;

    char str9[380];
    char str10[380];
	// cajas enteras
	sprintf(str9, "%s/testout-v13largo.unix.csv", ioPATH.c_str());
	sprintf(str10, "%s/testout-v13cuad.unix.csv", ioPATH.c_str());
	
    char str12[380];
    sprintf(str12, "%s/exportCPP.unix.txt", ioPATH.c_str());
    //cajas continuas

    char* argvz[] = { "DFS.exe",
        str9, str10, "", "",
        str12 };

    set<string> setSectores;
    if (argc == 1) {
        setSectores = cargaVectores(argvz, n1, g1, p1, Fa, e1);
    }
    else {
        if (argc == 6 /*25*/) {
            ioPATH = string(argv[3 /*19*/]);
            ioPATH2 = string(argv[3 /*19*/]);
            ioPATH3 = string(argv[3 /*19*/]);
            CUAD = argv[4 /*20*/][0] == '1';

            setSectores = cargaVectores(argv, n1, g1, p1, Fa, e1);
        }
        else {
            fprintf(stderr, "Incorrecto número de argumentos.\n");
            exit(1);
        }
    }
    
	ofstream outInd1(ioPATH + "/IndicadorCostosDistribucion.txt");
    ofstream outInd2(ioPATH + "/IndicadorCostosDistribucionInterplanta.txt");
    ofstream outInd3(ioPATH + "/IndicadorCostosDistribucionPrimario.txt");
    
	ofstream outInd4(ioPATH + "/IndicadorInFullKilos.txt");
    ofstream outInd5(ioPATH + "/IndicadorInFullKilosInterplanta.txt");
    ofstream outInd6(ioPATH + "/IndicadorInFullKilosPrimario.txt");
    
	ofstream outInd7(ioPATH + "/IndicadorInFullPallets.txt");
    ofstream outInd8(ioPATH + "/IndicadorInFullPalletsInterplanta.txt");
    ofstream outInd9(ioPATH + "/IndicadorInFullPalletsPrimario.txt");
    
	ofstream outInd10(ioPATH + "/IndicadorInNivelServicio.txt");
    ofstream outInd11(ioPATH + "/IndicadorInNivelServicioNodo.txt");
    
	ofstream outInd12(ioPATH + "/IndicadorRatioCostosKilos.txt");
    ofstream outInd13(ioPATH + "/IndicadorVisitaNodosPlanta.txt");
    ofstream outInd14(ioPATH + "/IndicadorCargaNodosPlanta.txt");
    ofstream outInd15(ioPATH + "/IndicadorCamionesNodosPlanta.txt");

    ofstream outInd16(ioPATH + "/IndicadoresGlobales.txt");
    
    ofstream outSalida(ioPATH + "/SalidaDetalle.txt");
    ofstream outSalida2(ioPATH + "/SalidaDetalle2.txt");
    ofstream outSalidaCabecera(ioPATH + "/transportes-cabecera.txt");
    ofstream outVenta(ioPATH + "/venta-traslado.txt");
    ofstream outDetalle(ioPATH + "/transportes-detalle.txt");
    
    ofstream outRutas(ioPATH + "/Rutas.txt");
    ofstream outCuadratura(ioPATH + "/Cuadratura.txt");
    
    ofstream outCarga(ioPATH + "/CargaCamiones.txt");
    ofstream outCapacidades(ioPATH + "/capacidades.txt");

	ofstream outCatIdx(ioPATH + "/catalogo-idx.txt");
	outCatIdx << "IndicadoresGlobales.txt;IndicadoresGlobales;" << endl;
	outCatIdx << "IndicadorCamionesNodosPlanta.txt;IndicadorCamionesNodosPlanta;" << endl;
	outCatIdx << "IndicadorCargaNodosPlanta.txt;IndicadorCargaNodosPlanta;" << endl;
	outCatIdx << "IndicadorVisitaNodosPlanta.txt;IndicadorVisitaNodosPlanta;" << endl;
	outCatIdx << "IndicadorRatioCostosKilos.txt;IndicadorRatioCostosKilos;" << endl;
	outCatIdx << "IndicadorInNivelServicioNodo.txt;IndicadorInNivelServicioNodo;" << endl;
	outCatIdx << "IndicadorInNivelServicio.txt;IndicadorInNivelServicio;" << endl;
	outCatIdx << "IndicadorInFullPalletsPrimario.txt;IndicadorInFullPalletsPrimario;" << endl;
	outCatIdx << "IndicadorInFullPalletsInterplanta.txt;IndicadorInFullPalletsInterplanta;" << endl;
	outCatIdx << "IndicadorInFullPallets.txt;IndicadorInFullPallets;" << endl;
	outCatIdx << "IndicadorInFullKilosPrimario.txt;IndicadorInFullKilosPrimario;" << endl;
	outCatIdx << "IndicadorInFullKilosInterplanta.txt;IndicadorInFullKilosInterplanta;" << endl;
	outCatIdx << "IndicadorInFullKilos.txt;IndicadorInFullKilos;" << endl;
	outCatIdx << "IndicadorCostosDistribucionPrimario.txt;IndicadorCostosDistribucionPrimario;" << endl;
	outCatIdx << "IndicadorCostosDistribucionInterplanta.txt;IndicadorCostosDistribucionInterplanta;" << endl;
	outCatIdx << "IndicadorCostosDistribucion.txt;IndicadorCostosDistribucion;" << endl;
	    
    vector<Edge_parts> SOB;
    vector<Pallet> SOBP;
    int LabelLastCam;
    
    cout << "HEURÍSTICA DE EMPAQUETAMIENTO" << endl;
// Actualizo estado de familias
    cout << "Actualiza estado de familias de productos" << endl;
    
    for(unsigned int p=0; p<Fa.size();p++)
        Fa[p].set_congelado(p1);
    
//Actualizo tiempos de viaje
    cout << "Estima información faltante de tiempos de viaje" << endl;
    for(unsigned gg=0; gg<e1.size(); gg++)
        e1[gg].AjustaTV(n1);
    
    cout << "Comienza proceso de carga" << endl;
    Cargando_pallets2R(g1, e1, n1, p1, SOB, SOBP,Fa,&LabelLastCam);
    cout << "Actualiza tiempos totales camiones" << endl;
    g1.Actualiza_tiempos_totales(e1, n1, LabelLastCam);
    
    // seteando schedules de nodos sucursales
    cout << "Programa itinerarios de camiones" << endl;
    ProgramaItinerarioCamiones(g1,e1,n1,LabelLastCam);
    
    vector<string> vv(setSectores.begin(), setSectores.end());

    cout << "Imprime reportes y computa indicadores" << endl;
    outRutas << g1.PrintRutas(e1,n1,p1, vv);
    outCuadratura << g1.PrintCuadratura(e1,n1,p1);
    
    //archivos para SAP
    salidaTransportesCabecera(outSalidaCabecera, g1, e1, n1, Fa);
    salidaVentaTraslado(outVenta,g1,e1,n1,p1);
    salidaTransporteDetalle(outDetalle, g1, e1, n1, p1, LabelLastCam, Fa);
    
    compute_indicadores(outInd1,outInd2,outInd3,outInd4,outInd5,outInd6,outInd7,outInd8,outInd9,outInd10,outInd11,outInd12,outInd13,outInd14,outInd15,outInd16,g1, e1, n1, p1);
    salida_detalle(outSalida, g1, e1, n1, p1);
    DetalleCargaCamiones(outCarga, g1, e1, n1, p1, Fa);
    salida_detalle2(outSalida2, g1, e1, n1, p1);
    //nuevo archivo para Bonny
    salida_capacidades(outCapacidades, g1, e1, n1, p1, Fa);
    
    return 0;
}



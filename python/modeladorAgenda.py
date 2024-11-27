# -*- coding: utf-8 -*-
import lecturaAG as lt
import csv

from gurobipy import *
from gurobipy import GRB
from gurobipy import Model
from gurobipy import quicksum
import numpy as np
from dateutil.relativedelta import relativedelta
from datetime import datetime, date, time, timedelta

####################################################################################################
# DESDE AQUI!!!
####################################################################################################
def leerCargaRutas(matriz):
    cargaRutas = {}
    origenes = {}

    for sl in matriz:
        ssi = ""
        if sl[7] == "1":
            ssi = "si"
        cargaRutas[tuple(sl[0:6])] = (float(sl[6]), ssi, float(sl[8]), sl[9], sl[10][2:])
        if not sl[1] in origenes.keys():
            origenes[sl[1]] = {}
        if not sl[4] in origenes[sl[1]].keys():
            origenes[sl[1]][sl[4]] = {}
        origenes[sl[1]][sl[4]][sl[5]] = 0

    return cargaRutas, origenes


def procesarNodosCentro(nodosTipoCentro, nodosCentro):
    for i in range(0, len(nodosCentro)):
        pref = nodosTipoCentro[i][0:2]
        nodo = nodosCentro[i]
        if pref == "Hu":
            pref = "Pl"
        try:
            nodo = int(nodosCentro[i])
        except:
            pass
        nodosCentro[i] = pref + str(nodo)


def buscarZDMO(e, nodosCentro, nodosZDMO):
    pos = -1
    for i in range(0, len(nodosCentro)):
        if str(nodosCentro[i]) == e:
            pos = i
            break
    return nodosZDMO[pos]
    

def convertirRutas(matriz, nodosCentro, nodosZDMO):
    mRtn = {}
    fc = {"Refrigerado":('F', 'light_yellow'), "Congelado":('C', 'pale_blue')}
    cargaRutas, origenes = leerCargaRutas(matriz)
    
    mRtn[0] = ["", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""]
    mRtn[1] = ["", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""]
    mRtn[2] = ["", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""]
    mRtn[3] = ["", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""]
    
    cntCol = 6
    for e in origenes.keys():
        mRtn[0][cntCol] = buscarZDMO(e, nodosCentro, nodosZDMO)
        for s in origenes[e].keys():
            for t in origenes[e][s].keys():
                mRtn[3][cntCol] = s[0:3] + " " + fc[t][0]
                origenes[e][s][t] = cntCol
                cntCol += 1
    
    cnt = 4
    mRtn[cnt] = ["", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""]
    eAnt = list(cargaRutas.keys())[0]
    for e in cargaRutas.keys():
        if e[0] != eAnt[0] or e[2] != eAnt[2] or e[3] != eAnt[3]:
            cnt += 1
            mRtn[cnt] = ["", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""]

 
        mRtn[cnt][0] = int(e[3])
        mRtn[cnt][1] = cargaRutas[e][3]
        mRtn[cnt][2] = cargaRutas[e][4]
        mRtn[cnt][3] = e[0]
        mRtn[cnt][4] = e[2].replace("P", " P")
        mRtn[cnt][5] = cargaRutas[e][1]
        mRtn[cnt][origenes[e[1]][e[4]][e[5]]] = float(cargaRutas[e][0])            
        eAnt = e
        
    return mRtn.values()



class Agenda():
    def __init__(self, pathLog):
        self.logAlertas = open(pathLog + "/alertas.log","w")
        self.logUsado = False
        

    def preparativos(self, catalogo, hoy):

        nodosTipoCentro = lt.extraerColumna(catalogo["Nodos:Nodos"], 0)
        nodosCentro = lt.extraerColumna(catalogo["Nodos:Nodos"], 1)
        nodosZDMO = lt.extraerColumna(catalogo["Nodos:Nodos"], 2)
        
        procesarNodosCentro(nodosTipoCentro, nodosCentro)
        catalogo["Rutas:Rutas"] = list(convertirRutas(catalogo["Rutas"], nodosCentro, nodosZDMO))

        
        anio = hoy.year
        mes = hoy.month
        dia = hoy.day
        #nodosTipoCentro = lt.extraerColumna(catalogo["Nodos:Nodos"], 0)
        #nodosCentro = lt.extraerColumna(catalogo["Nodos:Nodos"], 1)
        #nodosZDMO = lt.extraerColumna(catalogo["Nodos:Nodos"], 2)

        nodosVentanaA = lt.extraerColumna(catalogo["Nodos:Nodos"], 3)
        nodosVentanaB = lt.extraerColumna(catalogo["Nodos:Nodos"], 4)
        nodosTCarga = lt.extraerColumna(catalogo["Nodos:Nodos"], 5)
        nodosTDescarga = lt.extraerColumna(catalogo["Nodos:Nodos"], 6)
        nodosAndenesa = lt.extraerColumna(catalogo["Nodos:Nodos"], 7)
        nodosMultaMas2Hr = lt.extraerColumna(catalogo["Nodos:Nodos"], 8)
        nodosMultaFueraVentana = lt.extraerColumna(catalogo["Nodos:Nodos"], 9)
        nodosCostoInfactibilidadPlanta = lt.extraerColumna(catalogo["Nodos:Nodos"], 10)
        nodosCostoCambiarVentana = lt.extraerColumna(catalogo["Nodos:Nodos"], 11)

        horariosAndenesNodo = {}
        for i in range(0, len(catalogo["Nodos:Nodos"])):
            horariosAndenesNodo[nodosCentro[i]] = lt.extraerFilaDesde(catalogo["Nodos:Nodos"], i, 12)

        #####
        self.nAnden={}       
        for i in range(0, len(catalogo["Nodos:Nodos"])):
            for j in [0,1,48,49,96,97,144,145,192,193]:
                self.nAnden[nodosCentro[i],j]=horariosAndenesNodo[nodosCentro[i]][18]
                self.nAnden[nodosCentro[i],j+2]=horariosAndenesNodo[nodosCentro[i]][19]
                self.nAnden[nodosCentro[i],j+4]=horariosAndenesNodo[nodosCentro[i]][20]
                self.nAnden[nodosCentro[i],j+6]=horariosAndenesNodo[nodosCentro[i]][21]
                self.nAnden[nodosCentro[i],j+8]=horariosAndenesNodo[nodosCentro[i]][22]
                self.nAnden[nodosCentro[i],j+10]=horariosAndenesNodo[nodosCentro[i]][23]
                self.nAnden[nodosCentro[i],j+12]=horariosAndenesNodo[nodosCentro[i]][0]
                self.nAnden[nodosCentro[i],j+14]=horariosAndenesNodo[nodosCentro[i]][1]
                self.nAnden[nodosCentro[i],j+16]=horariosAndenesNodo[nodosCentro[i]][2]
                self.nAnden[nodosCentro[i],j+18]=horariosAndenesNodo[nodosCentro[i]][3]
                self.nAnden[nodosCentro[i],j+20]=horariosAndenesNodo[nodosCentro[i]][4]
                self.nAnden[nodosCentro[i],j+22]=horariosAndenesNodo[nodosCentro[i]][5]
            for j in [24,25,72,73,120,121,168,169]:
                self.nAnden[nodosCentro[i],j]=horariosAndenesNodo[nodosCentro[i]][6]
                self.nAnden[nodosCentro[i],j+2]=horariosAndenesNodo[nodosCentro[i]][7]
                self.nAnden[nodosCentro[i],j+4]=horariosAndenesNodo[nodosCentro[i]][8]
                self.nAnden[nodosCentro[i],j+6]=horariosAndenesNodo[nodosCentro[i]][9]
                self.nAnden[nodosCentro[i],j+8]=horariosAndenesNodo[nodosCentro[i]][10]
                self.nAnden[nodosCentro[i],j+10]=horariosAndenesNodo[nodosCentro[i]][11]
                self.nAnden[nodosCentro[i],j+12]=horariosAndenesNodo[nodosCentro[i]][12]
                self.nAnden[nodosCentro[i],j+14]=horariosAndenesNodo[nodosCentro[i]][13]
                self.nAnden[nodosCentro[i],j+16]=horariosAndenesNodo[nodosCentro[i]][14]
                self.nAnden[nodosCentro[i],j+18]=horariosAndenesNodo[nodosCentro[i]][15]
                self.nAnden[nodosCentro[i],j+20]=horariosAndenesNodo[nodosCentro[i]][16]
                self.nAnden[nodosCentro[i],j+22]=horariosAndenesNodo[nodosCentro[i]][17]

        for i in range(0, len(catalogo["Nodos:Nodos"])):
            for j in range(0,216):
                if self.nAnden[nodosCentro[i],j]==None:
                    self.nAnden[nodosCentro[i],j]=0
           
        #####

        ####################################################################################################
        # HASTA AQUI!!!    
        ####################################################################################################
        

#        print('Comenzamos a leer tarifas')

        tarifasOri = lt.extraerColumna(catalogo["Tarifas:Tiempos de Viaje"], 0)
        tarifasDest = lt.extraerColumna(catalogo["Tarifas:Tiempos de Viaje"], 1)
        tarifasTiempoViaje = lt.extraerColumna(catalogo["Tarifas:Tiempos de Viaje"], 2)

#        print(tarifasDest)

        tarifasDestino = [] 
        for i in range(len(tarifasDest)):
            if isinstance(tarifasDest[i],float):
               tarifasDestino.append(str(tarifasDest[i])[:-2])
            else:
               tarifasDestino.append(tarifasDest[i])

        tarifasOrigen =[] 
        for i in range(len(tarifasOri)):
            if isinstance(tarifasOri[i],float):
               tarifasOrigen.append(str(tarifasOri[i])[:-2])
            else:
               tarifasOrigen.append(tarifasOri[i])


        self.rutasNroCamion = lt.extraerColumna(catalogo["Rutas:Rutas"], 0)[4:]
        rutasTipoViaje = lt.extraerColumna(catalogo["Rutas:Rutas"], 1)[4:]
        rutasCamionSuminitrador = lt.extraerColumna(catalogo["Rutas:Rutas"], 2)[4:]
        rutasCodigoRuta = lt.extraerColumna(catalogo["Rutas:Rutas"], 3)[4:]
        rutasTipoCamion = lt.extraerColumna(catalogo["Rutas:Rutas"], 4)[4:]
        rutasSeparador = lt.extraerColumna(catalogo["Rutas:Rutas"], 5)[4:]


        sectores = ["Cecina", "Cerdo", "Elaborado", "Genética", "Hortalizas y Frutas", "Pavo", "Pollo", "Prod. del Digestor", "Salmón", "Vacuno"]
        mSectores = {}
        for e in sectores:
            mSectores[e[:3]]=e
        mEstados = {'F': "Fresco", 'C':"Congelado"}
        planta = ""
        cargas = {}       # detalle de cargas en viajes que visitan a P029

#        print(catalogo["Rutas:Rutas"][0])
#        print(catalogo["Rutas:Rutas"][3])
#
#    en cargas[v,i,se,es] los productos por sector y estado cargados en i para cada viaje v que pase por P029
#
        for i in range(4, len(catalogo["Rutas:Rutas"])):
#            print(catalogo["Rutas:Rutas"][i])
            for j in range(6, len(catalogo["Rutas:Rutas"][i])):
                if catalogo["Rutas:Rutas"][0][j] != "":
                    planta = catalogo["Rutas:Rutas"][0][j]
                sectorEstado = catalogo["Rutas:Rutas"][3][j].split(" ") 
                if 'PlP029' in rutasCodigoRuta[i-4].split('-'):
                    try:
                       cargas[self.rutasNroCamion[i-4],nodosCentro[nodosZDMO.index(planta)], mSectores[sectorEstado[0]], mEstados[sectorEstado[1]]] = float(catalogo["Rutas:Rutas"][i][j])
                    except:
                       cargas[self.rutasNroCamion[i-4],nodosCentro[nodosZDMO.index(planta)], mSectores[sectorEstado[0]], mEstados[sectorEstado[1]]] = 0.0

       
#
#      clave es x[0] 2^0 + x[1] 2^1 + x[2] 2^2 
#
#      donde x[0] = 1  si  Cerdo Fresco
#            x[1] = 1  si  Pollo Fresco
#            x[2] = 1  si  Congelado
#
        clave = {}
        for v in self.rutasNroCamion:
           clave[v] = [0, 0, 0]
           for i in nodosCentro:
              for se in sectores:
                 for es in mEstados.values():
                    if (v,i,se,es) in cargas and cargas[v,i,se,es]>0.0: 
                       if se=='Cerdo' and es=='Fresco':
                          clave[v][0] = 1
                       if se=='Pollo' and es=='Fresco':
                          clave[v][1] = 1
                       if es=='Congelado':
                          clave[v][2] = 1

#        print('clave:', len(clave), clave ) 
#        print('cargas.keys: ', cargas.keys(), len(cargas.keys()))
#        print('cargas', len(cargas), cargas) 

           
        # Definir conjuntos de viajes conCerF, conPolF y conCong que visitan un edificio y las versions PS que visitan mas de uno.
        self.conCerF = {}
        self.conPolF = {}
        self.conCong = {}
        self.conCerFPS = {}
        self.conPolFPS = {}
        self.conCongPS = {}

        for v in clave: 
            clax = clave[v][0]+2*clave[v][1]+4*clave[v][2]
            if clax == 1:
               self.conCerF[v] = None
            elif clax == 2:
               self.conPolF[v] = None
            elif clax == 4:
               self.conCong[v] = None
            elif clax == 3:
               self.conCerFPS[v] = None
               self.conPolFPS[v] = None
            elif clax == 5:
               self.conCerFPS[v] = None
               self.conCongPS[v] = None
            elif clax == 6:
               self.conPolFPS[v] = None
               self.conCongPS[v] = None
            elif clax == 7:
               self.conCerFPS[v] = None
               self.conPolFPS[v] = None
               self.conCongPS[v] = None

#        print('Cerdo Fresco')
#        for v in conCerF:
#            print(v)
#        print('Pollo Fresco')
#        for v in conPolF:
#            print(v)
#        print('Congelado')
#        for v in conCong:
#            print(v)
#        print('Cerdo Fresco plus')
#        for v in conCerFPS:
#            print(v)
#        print('Pollo Fresco plus')
#        for v in conPolFPS:
#            print(v)
#        print('Congelado plus')
#        for v in conCongPS:
#            print(v)
#


        #print("FIRSTRY!!")
        #print("----------->", hoy)

        self.trip={}
        self.locvisit={}
        self.pred={}
        for i in range(len(self.rutasNroCamion)):
            self.trip[self.rutasNroCamion[i]]=rutasCodigoRuta[i].split('-')
            for j in range(len(self.trip[self.rutasNroCamion[i]])):
                if self.trip[self.rutasNroCamion[i]][j].isnumeric():
                    self.trip[self.rutasNroCamion[i]][j]=int(self.trip[self.rutasNroCamion[i]][j])
                    self.locvisit[int(self.trip[self.rutasNroCamion[i]][j])]=None
                else:
                    self.locvisit[self.trip[self.rutasNroCamion[i]][j]]=None
            if rutasCamionSuminitrador[i] != '':
                self.pred[self.rutasNroCamion[i]]=rutasCamionSuminitrador[i].replace('(',' ').replace(')',' ').replace('-',' ').split()
            else:
                self.pred[self.rutasNroCamion[i]]=[]
        
        location={}
        self.service={}
        capacanden={}
        self.LBound={}
        self.UBound={}
        LBaux={}
        UBaux={}
        self.excesswait={}
        self.delaywindow={}
        self.beforewindow={}
        self.extendtwcost={}
        self.bigcost={}
        for i in range(len(nodosCentro)):
            if type(nodosVentanaA[i])==float: 
                LBaux[i]=nodosVentanaA[i]*24*60
            else:
                LBaux[i]=0
            if type(nodosVentanaB[i])==float: 
                UBaux[i]=nodosVentanaB[i]*24*60
            else:
                UBaux[i]=1439
        for i in range(len(nodosCentro)):
            location[nodosCentro[i]]=None
            self.service[nodosCentro[i]]=relativedelta(hours=int(nodosTCarga[i]))   #TCarga en hrs.
            capacanden[nodosCentro[i]]=int(nodosAndenesa[i])
            self.LBound[nodosCentro[i]]=datetime(anio,mes,dia,hour=int(LBaux[i]//60),minute=int(LBaux[i]%60))   
            self.UBound[nodosCentro[i]]=datetime(anio,mes,dia,hour=int(UBaux[i]//60),minute=int(UBaux[i]%60)) 

    #
    #  Estos parametros se deben leer de los datos en archivo Nodos
    #       
            self.excesswait[nodosCentro[i]]=nodosMultaMas2Hr[i]
            self.delaywindow[nodosCentro[i]]=nodosMultaFueraVentana[i]
            self.beforewindow[nodosCentro[i]]=nodosMultaFueraVentana[i] 
    #
    #	solo Su y Ve pueden cambiar ventana, solo Pl puede tener una variable para identificar infactibiliades de capacidad 
    #       los costos estan todos en la misma columna (costo cambio de ventana y costo de infactibilidad que es mayor)
    #       las variables que no se utilizan, se definen e == a 0.
            self.bigcost[nodosCentro[i]] = nodosCostoInfactibilidadPlanta[i]
            self.extendtwcost[nodosCentro[i]]= nodosCostoCambiarVentana[i]

          
        self.travelt={}
        for i in range(len(tarifasOrigen)):
            for k in self.locvisit:
                for l in self.locvisit:
                    if str(k)[2:]==tarifasOrigen[i] and str(l)[2:]==tarifasDestino[i]:
                        if isinstance(tarifasTiempoViaje[i],float):
                             self.travelt[k,l]=relativedelta(hours=tarifasTiempoViaje[i])
                        else:
                             print('ERROR travel time is not float between ', k,' and ', l)
                             self.logAlertas.write('ERROR travel time is not float between ' + str(k) +' and ' + str(l) + " [hours=6]\n")
                             self.logUsado = True
                             self.travelt[k,l]=relativedelta(hours=6)
                
           

        #
        #  Check that data for routes is available
        #

        for i in self.locvisit:
            if i not in location:
                print('ERROR no data for location ', i,' that is visited')
                self.logAlertas.write('ERROR no data for location '+ str(i) +' that is visited\n')
                self.logUsado = True
        for v in self.trip:
            for i in range(len(self.trip[v])-1):
                ii = self.trip[v][i]
                jj = self.trip[v][i+1]
                if (ii,jj) not in self.travelt:
                    print('ERROR no travel time between ', ii,' and ', jj, ' that are visited')
                    self.logAlertas.write('ERROR no travel time between ' + str(ii) +' and ' + str(jj) + ' that are visited' + " [hours=4]\n")
                    self.logUsado = True
                    self.travelt[ii,jj] = relativedelta(hours=4)

        self.logAlertas.close()
        return self.logUsado
        
    
    
    def agendar(self, tm):   
        for v in self.trip:
            for i in range(len(self.trip[v])-1):
                ii = self.trip[v][i]
                jj = self.trip[v][i+1]
                print(ii,jj,self.travelt[ii,jj],self.travelt[ii,jj].days, self.travelt[ii,jj].hours, self.travelt[ii,jj].minutes)	            

        Ttime={}
        Sumtime={}
        for i in range(216): #original 60
            Ttime[i]=relativedelta(minutes=30)
            Sumtime[i]=(relativedelta(minutes=30)*(i))

        TimeRecor={}
        for i in range(48): #original 60, para restriccion de nodo inicial
            TimeRecor[i]=relativedelta(minutes=30)
            
        loctriptime={}
        for t in Ttime:
            for v in self.trip:
                for i in self.trip[v]:
                    loctriptime[i,v,t]=None

        loctrip={}
        for v in self.trip:
            for i in self.trip[v]:
                loctrip[i,v]=None
                
        loctime={}
        for t in Ttime:
            for v in self.trip:
                for i in self.trip[v]:
                    loctime[i,t]=None

#
#  FOP:  check conCong, conPolF, conCerF are something here
#
        vs = {}
        for i,v in loctrip:
#            print(i,v)
            if i== 'PlP029' and (v in self.conCong or v in self.conPolF or v in self.conCerF):
               vs[i,v] = max(self.service[i].hours*60+self.service[i].minutes-60,60)
#               print('Short stay at P029')
            else:
               vs[i,v] = self.service[i].hours*60+self.service[i].minutes
#
#



        #tm = Model('ppl')
        #tm.setParam('LogFile', pathLog + '/Lognobaja.log')
        
        #
        # definition of variables
        #
        tw = {}
        for i,v,t in loctriptime:
            tw[i,v,t] = tm.addVar(vtype="B",lb=0,name='tw_%s_%s_%s' % (i, v, t))

        tf =  {}
        tfmil={}
        c =   {}
        alp = {}
        bet = {}
        dlt = {}
        for i,v in loctrip:
            tf[i,v] = tm.addVar(vtype="C",lb=0,name='tf_%s_%s' % (i, v))
            tfmil[i,v] = tm.addVar(vtype="C",lb=0,name='tfmil_%s_%s' % (i, v))
            c[i,v] = tm.addVar(vtype="I",lb=0,name='c_%s_%s' % (i, v))
            alp[i,v] = tm.addVar(vtype="C",lb=0,name='alp_%s_%s' % (i, v))
            bet[i,v] = tm.addVar(vtype="C",lb=0,name='bet_%s_%s' % (i, v))
            dlt[i,v] = tm.addVar(vtype="C",lb=0,name='dlt_%s_%s' % (i, v))

        x = {}
        y = {}
        for i in self.locvisit:
            x[i] = tm.addVar(vtype="C",lb=0,name='x_%s' % i)
            y[i] = tm.addVar(vtype="C",lb=0,name='y_%s' % i)

        gamma={}
        for i,t in loctime:
            gamma[i,t] = tm.addVar(vtype="I",lb=0,name='gamma_%s_%s' % (i, t))


            #
            #  definition of Constraints
            #
        for v in self.trip:
            for i in self.locvisit:
                if i in self.trip[v]:
                    tm.addConstr(quicksum(tw[i,v,t] for t in Ttime) == 1) 
                    tm.addConstr(quicksum((30*(t))*tw[i,v,t] for t in Ttime) == tf[i,v]) 
                     
        for v in self.trip:
            for i in range(len(self.trip[v])-1):
                ii = self.trip[v][i]
                jj = self.trip[v][i+1]
                tm.addConstr(tf[ii,v]+vs[ii,v]+(self.travelt[ii,jj].days*24*60+self.travelt[ii,jj].hours*60+self.travelt[ii,jj].minutes)+alp[jj,v]==tf[jj,v])




   # Restriccion Capacidades, FOP:  check conCong, conPolF, conCerF, et al  are readable here
        for i in self.locvisit:
            intervalos=int((self.service[i].hours*60+self.service[i].minutes)/30)
            for t in Ttime:
    #	        print(t,i,intervalos)
                if i!='PlP029':
                    tm.addConstr(quicksum(tw[i,v,s] for v in self.trip for s in range(max(t-intervalos+1,0),t+1) if (i,v,s) in loctriptime) <= self.nAnden[i,t]+gamma[i,t] )
                if i=='PlP029': # RESTRICCIONES 3 EDIFICIOS LO MIRANDA
                # en LoMiranda PlP029, un camion que solo visita un edificio demora 2hrs en vez de 3 hrs. Fijado 1 hr menos que intervalos 
                    intersolo = max(intervalos-2,2)  
                    tm.addConstr(quicksum(tw[i,v,s] for v in self.conCerF for s in range(max(t-intersolo+1,0),t+1) if (i,v,s) in loctriptime) + quicksum(tw[i,v,s] for v in self.conCerFPS for s in range(max(t-intervalos+1,0),t+1) if (i,v,s) in loctriptime) <= self.nAnden['PlP029C',t] + gamma[i,t] )
                    tm.addConstr(quicksum(tw[i,v,s] for v in self.conPolF for s in range(max(t-intersolo+1,0),t+1) if (i,v,s) in loctriptime) + quicksum(tw[i,v,s] for v in self.conPolFPS for s in range(max(t-intervalos+1,0),t+1) if (i,v,s) in loctriptime) <= self.nAnden['PlP029A',t] + gamma[i,t] )
                    tm.addConstr(quicksum(tw[i,v,s] for v in self.conCong for s in range(max(t-intersolo+1,0),t+1) if (i,v,s) in loctriptime) + quicksum(tw[i,v,s] for v in self.conCongPS for s in range(max(t-intervalos+1,0),t+1) if (i,v,s) in loctriptime)  <= self.nAnden['PlP029F',t] + gamma[i,t] )



        for i,v in loctrip:
            tm.addConstr((tf[i,v]+1080 == tfmil[i,v]))

        for i,v in loctrip:
            tm.addConstr(c[i,v]*1440 >= tfmil[i,v]+vs[i,v])

    #	for i,v in loctrip:
    #	    for j in range(len(self.trip[v])-1):
    #	            ii = self.trip[v][j]
    #	            jj = self.trip[v][j+1]
    #	            if i==self.trip[v][0]:
    #	                tm.addConstr(c[i,v]*1440 >=  tfmil[i,v]+vs[i,v])
    #	            else:
    #	                tm.addConstr(c[jj,v]*1440 >=  tfmil[ii,v]+vs[ii,v]+alp[jj,v]+(self.travelt[ii,jj].days*24*60+self.travelt[ii,jj].hours*60+self.travelt[ii,jj].minutes))
            

        for i,v in loctrip:
                UBaux = self.UBound[i].hour*60+self.UBound[i].minute
                LBaux = self.LBound[i].hour*60+self.LBound[i].minute
                if UBaux < LBaux:
                   UBaux = UBaux+1440
                tm.addConstr( UBaux+dlt[i,v]+(c[i,v]-1)*1440 >= tfmil[i,v])
                tm.addConstr(tfmil[i,v] >= LBaux-bet[i,v]+(c[i,v]-1)*1440)
                        
        for l in self.pred:
            for kk in range(1,len(self.pred[l])):
                vc = int(self.pred[l][kk])
                ii = self.trip[vc][1]   # delivery location of consolidation self.trip
                tm.addConstr(tf[ii,vc]+vs[ii,vc] <= tf[ii,l])    

        # Restriccion añadida, para que empiece en el primer dia
        for i,v in loctrip: 
            if i == self.trip[v][0]:
                tm.addConstr(quicksum(tw[i,v,t] for t in TimeRecor) == 1)
                    
        # Definiendo variables igual a 0
        for v in self.trip:
            i = self.trip [v][0]
            j = self.trip [v][1]
            aux = int((self.travelt[i,j].minutes+self.travelt[i,j].hours*60+self.travelt[i,j].days*24*60)/30)
            for t in range(49,216): #ORIGINAL 48
                tm.addConstr(tw[i,v,t]==0)
            aux2 = min(48, aux) 
            for t in range(0,aux): #ORIGINAL aux2
                tm.addConstr(tw[j,v,t]==0)
            aux3 = aux+48+10 #con el +10 le doy espacio a alpha
            for t in range(aux3,216):
                tm.addConstr(tw[j,v,t]==0)
                

    #     no se puede ampliar ventana de tiempo en Plantas  (i comienza con Pl)
    #      
        for i in self.locvisit:
            if str(i)[:2]=='Pl':
                tm.addConstr(x[i]==0)
                tm.addConstr(y[i]==0)

    #     no se puede aumentar el numero de andenes ni acelerar operacion en Sucursales/Venta directa  (i comienza con Su o Ve)
    #
        for i,t in loctime:
            if str(i)[:2]!='Pl':
                tm.addConstr(gamma[i,t]==0)

            #
            #  definition of Objective function
            #
        
        tm.setObjective(quicksum(self.excesswait[i]*alp[i,v] for (i,v) in loctrip)+
        quicksum(self.delaywindow[i]*dlt[i,v] for (i,v) in loctrip)+
        quicksum(self.beforewindow[i]*bet[i,v] for (i,v) in loctrip)+
        quicksum(self.extendtwcost[i]*x[i] for i in self.locvisit) + 
        quicksum(self.extendtwcost[i]*y[i] for i in self.locvisit) +
        quicksum(c[i,v] for (i,v) in loctrip) +
        quicksum(self.bigcost[i]*gamma[i,t] for (i,t) in loctime ), GRB.MINIMIZE)



            #
            #  End Model, Solve optimization Problem
            #
        tm.optimize()

        outpath = tm.Params.LogFile[:-14]     # removing '/Lognobaja.log' from LogFile to keep path with output

        if tm.status >= 3 and tm.status <=4:
            tm.computeIIS()
            tm.write(outpath + '/capacprob.ilp')
            #raise ValueError("Error en optimización. Modelo de optimización de camiones es infactible.")
            print("Error en optimización. Modelo de agendamiento de camiones es infactible.")


        # return tf, alp
            ##########

            #
            #   PRINT SOLUTION
            #


        var_names = []
        var_values = []
        var_names_inf = []
        var_values_inf = []

        var_inf = ['a', 'b', 'd', 'g', 'x', 'y', 'z']   # variables: alp, bet, dlt, gamma, x, y, z
        for var in tm.getVars():
            var_names.append(str(var.varName))
            var_values.append(var.X)
            if str(var.varName)[:1] in var_inf and var.X != 0:
                var_names_inf.append(str(var.varName))
                var_values_inf.append(var.X)

        with open(outpath+'/solAgendamiento.csv', 'w') as myfile:
            wr = csv.writer(myfile, lineterminator = '\n')
            wr.writerows(zip(var_names, var_values))
        myfile.close()

        with open(outpath+'/solVariablesINF.csv', 'w') as myfile:
            wr = csv.writer(myfile, lineterminator = '\n')
            wr.writerows(zip(var_names_inf, var_values_inf))
        myfile.close()

            #
            #
            #########

        #print("TERMINÓ EL MODELO")
        print('-------------------------')
        #print("Haciendo lista con resultados")
        # El formato es Viaje, nodo, HoraLlegada, HoraSalida
        # Conversion a lista
        # Paso previo, calculo ts, tiempo de salida
        TpoEntrada_Min={}
        TpoSalida_Min={}
        TpoEntrada_Seg={}
        TpoSalida_Seg={}
        for v in self.trip:
            for i in self.trip[v]:
                if tf[i,v].x>=0.0:
                    TpoEntrada_Min[i,v]=tf[i,v].x
                    TpoSalida_Min[i,v]=tf[i,v].x+vs[i,v]
        for var in loctrip:
            TpoEntrada_Seg[var]=(TpoEntrada_Min[var]+1080)*60-86400
            TpoSalida_Seg[var]=(TpoSalida_Min[var]+1080)*60-86400
            
        itinerario={}
        k=1
        for v in self.trip:
            for i in self.trip[v]:
                text2=[]
                text2.append(str(int(TpoEntrada_Seg[i,v])))
                text2.append(str(int(TpoSalida_Seg[i,v])))       
                itinerario[str(int(self.rutasNroCamion[k-1])), str(i)] = text2
            k=k+1
        

        return itinerario

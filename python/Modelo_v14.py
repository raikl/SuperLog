# -*- coding: utf-8 -*-
"""
@author: Macarena V. Osorio A.
"""

import csv
from gurobipy import *
from gurobipy import GRB
from gurobipy import Model
from gurobipy import quicksum
import sys as sys
import math 
from datetime import date
import frescura_v11 as fres
import recorte_v13 as rcte
import cuadratura_v14 as cuad


class Modelo:
    def modelar(self):
        #DATA DESDE LOSEXCELS 
        IREAL = self.IREAL
        JREAL = self.JREAL
        JLT = self.JLT
        OETIQ = self.OETIQ
        HO = self.HO
        D = self.D
        HD = self.HD
        JD = self.JD
        H = self.H
        SLH = self.SLH
        CAJASPALLET = self.CAJASPALLET
        HPESO = self.HPESO
        PESOCAJA = self.PESOCAJA
        CAP = self.CAP
        KG = self.KG
        K = self.K
        PJH = self.PJH
        HPRECIOJH = self.HPRECIOJH
        JPRECIOJH = self.JPRECIOJH
        CH = self.CH
        HCOSTOH = self.HCOSTOH
        CV2DATOS = self.CV2DATOS
        ICV2 = self.ICV2
        JCV2 = self.JCV2
        KCV2 = self.KCV2

        #
        #  PARAMETROS FIJOS
        #
        #ALFA
        alfa=0.2
        #
        #  dia del despacho  (para frescura de producto)
        dayrun = self.fecha#date(2020,3,19)

        #
        #  grupos de productos, son <= 6 pues son tres LT (1,2,y 3) y 2 niveles de servicio por producto (CCS y no)
        SETD = range(6)

        #
        #  Nodos de consolidacion
        #
        #cons={}
        #cons['P001..'] = 'P001..'
        #cons['P002..'] = 'P002..'
        #cons['P008..'] = 'P008..'
        cons = self.cons

        #CONJUNTO DE FAMILIAS:
            #1-> FRESCO: Cerdo, Pavo, Pollo y Vacuno.
            #2-> FRESCO: Cecina.
            #3-> CONGELADO: Cerdo, Pavo, Pollo y Vacuno.
            #4-> CONGELADO: Hortalizas y Frutas.
            #5-> CONGELADO: Elaborado y Salmón.
        F = self.F#[1,2,3,4,5]
        congelado = self.congelado#[3,4,5]
        fresco = self.fresco#[1,2]

        #PRODUCTOS Y FAMILIA
        w = self.w


        ######################################################################
        ## COLOCAR MODIFICACIONES DESPUES DE ESTE COMENTARIO
        ######################################################################       





        # GRAFO REAL
        #  I : plantas
        #  J : sucursales

        J={}
        JS={}
        traveltime = {}
        for j in range(len(JREAL)):
            jj = JREAL[j]
            J[jj]=jj
            traveltime[jj]=JLT[j]
            if str(jj)[0:2]=='Su':
                JS[jj+'S'] = jj+'S'
                traveltime[jj+'S']=JLT[j]

        JuJS = {**J, **JS}   
            
        I={}
        for i in IREAL:
            I[i]=i 
            
        ad = {**I, **J}
        adJS = {**ad, **JS}


        punto='.'
            
        I1={}
        I2={}
        I0={}
        #nc = {}
        for i in I:
            I1[i]=str(i)+punto
            I2[i]=str(i)+punto+punto
            I0[I1[i]]=i
            I0[I2[i]]=i
        #    nc[I1[i]] = None
        #    nc[I2[i]] = None

            
        #nc = {**nc, **J}    # NOT needed ???  check


        #
        #  solo los arcos con costo finito
        ARCV = {}

        for indx in range(len(KCV2)):
            ARCV[ICV2[indx],JCV2[indx]] = None
            
#
#   arcos planta planta solo para los viajes de consolidacion
#   resto va en viaje directo l,i,j
        arcon= {}
        arcii = {}
        for i1 in (I1.values()):
            for i2 in (I2.values()):
                if  I0[i1] != I0[i2]:
                    if (I0[i1],I0[i2]) in ARCV:
                        arcii[i1,i2]=None
                        if i2 in cons:
                            arcon[i1,i2]=None


        arcij= {}
        for i2 in (I2.values()):
            for j in (J.values()):
                if I0[i2] != j:
                    if (I0[i2],j) in ARCV:
                        arcij[i2,j]=None
                
        acdir= {}
        for l in I:
            for i in I:
                for j in J:
                    if (l,i) in ARCV and (i,j) in ARCV:
                        acdir[I1[l],I2[i],j]= None
                
        citypair = {}  
        pareadas = {}      
        for indx in range(len(self.compartidosOrigen)):
            citypair[self.compartidosOrigen[indx], self.compartidosDestino[indx]] = None
            pareadas[self.compartidosOrigen[indx]] = None
            pareadas[self.compartidosDestino[indx]] = None


        arcijj = {}
        for i in I:
            for (j1,j2) in citypair:
                if i!= j1 and (i,j1) in ARCV and (j1,j2) in ARCV:
                    arcijj[I2[i],j1,j2]=None
                if i!= j2 and (i,j2) in ARCV and (j2,j1) in ARCV:
                    arcijj[I2[i],j2,j1]=None

                    
        #CV  -  costos de arcos
        #
        cvdatos = {} 
        ARCVK = {}      # conjunto de arcos para y, sobre A_c y para algunos camiones
        cv2datos = {} 
        ARCV2K = {}
        cvsdatos = {}
        ARCVSK = {}

        for cv in range(len(CV2DATOS)):
            if ICV2[cv] in I:
                if JCV2[cv] in I and (I1[ICV2[cv]],I2[JCV2[cv]]) in arcii:
                    cvdatos[I1[ICV2[cv]],I2[JCV2[cv]],KCV2[cv]]=CV2DATOS[cv]
                    if I2[JCV2[cv]] in cons:
                        ARCVK[I1[ICV2[cv]],I2[JCV2[cv]],KCV2[cv]] = None
                if JCV2[cv] in J and (I2[ICV2[cv]],J[JCV2[cv]]) in arcij:
                    cvdatos[I2[ICV2[cv]],J[JCV2[cv]],KCV2[cv]]=CV2DATOS[cv]
                    ARCVK[I2[ICV2[cv]],J[JCV2[cv]],KCV2[cv]] = None
            if ICV2[cv] in J and JCV2[cv] in J:
                cvdatos[J[ICV2[cv]],J[JCV2[cv]],KCV2[cv]] = CV2DATOS[cv]
                
        for (l,i,j) in acdir:
            for k in K:
                if (l,i,k) in cvdatos and (i,j,k) in cvdatos:
                    cv2datos[l,i,j,k]=cvdatos[l,i,k]+cvdatos[i,j,k]
                    ARCV2K[l,i,j,k] = None 

        for (i,j1,j2) in arcijj:
            for k in K:
                if (i,j1,k) in cvdatos and (j1,j2,k) in cvdatos:
                    cvsdatos[i,j1,j2,k] = cvdatos[i,j1,k]+cvdatos[j1,j2,k]
                    ARCVSK[i,j1,j2,k] = None

        #CAPACIDAD VOL
        cap = {}
        for k in range(len(CAP)):
            cap[K[k]]=CAP[k]
        
        # limites pallets congelados en camiones mixtos
        mincap = {}
        maxcap = {}
        for k in range(len(self.MinPallets)):
            mincap[K[k]]=self.MinPallets[k]
            maxcap[K[k]]=self.MaxPallets[k]
        
        #CAPACIDAD KG
        kg = {}
        for k in range(len(KG)):
            kg[K[k]]=KG[k]


        #CAMBIAR A KG
        pesocaja = {}
        for h in range(len(PESOCAJA)):
            pesocaja[HPESO[h]]=PESOCAJA[h]
            
        cajaspallet = {}
        for h in range(len(CAJASPALLET)):
            cajaspallet[HPESO[h]]=CAJASPALLET[h]


        #COSTO de produccion
        c = {}
        for h in range(len(CH)):
            c[HCOSTOH[h]]=CH[h]

        #PRECIO de venta   
        pre = {}
        for pjh in range(len(PJH)):
            pre[JPRECIOJH[pjh],HPRECIOJH[pjh]]=PJH[pjh]


        #DEMANDA
        dori = {}
        for demanda in range(len(D)):
            dori[HD[demanda],JD[demanda]]=D[demanda]
 
        #
        #   se debe tener un precio para cada demanda (h,j).  
        #   Si no existe el precio en un nodo:  
        #      1. usamos el precio del otro canal si existe
        #      2. usamos el precio promedio del producto si existe
        #      3. si no ingresamos un precio igual al costo.
        #      4. si no tiene costo  pre[j,h] = 0
        #                             
        for (h,j) in dori:
            if (j,h) not in pre:
                print('fixed pre[',j,',',h,']')
                if str(j)[-1:]=='S' and (str(j)[:-1],h) in pre:
                    pre[j,h]=pre[str(j)[:-1],h]
                    print('type 1')
                elif (str(j)+'S',h) in pre:
                    pre[j,h]=pre[str(j)+'S',h]
                    print('type 1b')
                else:
                    priaux = { pre[jj,h] for jj in JuJS if (jj,h) in pre }
                    if len(priaux)>0:
                        pre[j,h]=sum(priaux)/len(priaux)
                        print('type average')
                    elif h in c:
                        pre[j,h]=c[h]
                        print('type cost')
                    else:
                        pre[j,h]=0
                        print('type 0')
            
        #    
        #OFERTA
        #        
        #  ostock[h,d,i]  oferta en planta i de producto h con d dias mas de vida        
        #  dayrun = date(2020,3,19)
        #

        SL = {}
        for indx in range(len(H)):
            SL[H[indx]]=SLH[indx]
                   

        ostock = {}
        for indx in range(len(HO)):
            if HO[indx] in H:
                daystr = OETIQ[indx]
                if isinstance(daystr, str):
                    daystr = daystr.split('.')
                    d0 = date(int(daystr[2]),int(daystr[1]),int(daystr[0]))
                else:
                    d0 = date(daystr.year, daystr.month, daystr.day)
                delta = dayrun-d0
                for e in IREAL:
                    if (HO[indx],SL[HO[indx]]-delta.days,e) not in ostock.keys():
                        ostock[HO[indx],SL[HO[indx]]-delta.days,e]=self.OPS[e][indx]
                    else:
                        ostock[HO[indx],SL[HO[indx]]-delta.days,e]+=self.OPS[e][indx]
                    
        #
        #  need LT[h,j]  = 3 + travel time
        #   supermercados tienen requisito de 0.75*SL   parejo !  check this
        #
        LT = {}
        for (h,j) in dori:
            if j in self.minSlNodos:
                if self.minSlNodos[j]>1:
                    LT[h,j]=self.minSlNodos[j]+traveltime[j]
                else:
                    LT[h,j]=math.ceil(SL[h]*self.minSlNodos[j])+traveltime[j]

        #
        #  to check if a product infeasibility makes sense
        #
        #for (h,d,i) in ostock:
        #    if h==1020598 and ostock[h,d,i]>0:
        #        print(h,d,i,ostock[h,d,i])
        #for (h,j) in dori:
        #    if h==1020598 and dori[h,j]>0:
        #        print(h,j,LT[h,j],dori[h,j])
        #input()
    
        #
        #   Definir dfx y ofx que tiene demanda y oferta por grupos secuenciales 
        #   dfx[h,ds,j]  es satisfecha por sum(ofx[h,dsi,ii] con dsi>= ds)
        #
        #   oferta ofx[h,0,i] cuando no hay dfx[h,0,j] es producto basura
        #       aa = sum(ofx[hi,dsi,ii] for (hi,dsi,ii) in ofx if hi==h and dsi==0)
        #       bb = sum(dfx[hi,dsi,jj] for (hi,dsi,jj) in dfx if hi==h and dsi==0)
        #       if aa >0 and bb == 0:
        #           print(aa, ' cajas de producto ', h, ' estan vencidas')
        #
        #   se define tambien infdem[h] cantidad de demanad de h insatisfecha con frescura
        #
        
        #### exec(open("frescura.py").read())
        dfx, ofx, limd, infdem, infdemg = fres.frescura(H, JuJS, dori, LT, ostock, I, self.pathLogLogistica)
        
        dfxfull = {}
        for (h,g,j) in dfx:
            dfxfull[h,g,j] = dfx[h,g,j]

        infdemgfull = {}
        for (h,g) in infdemg:
            infdemgfull[h,g] = infdemg[h,g]
            
        dfx, infdemg = rcte.recorte(J, dfx, pesocaja, cajaspallet, pre, c, infdemg, citypair, pareadas, self.compartidosCamiones, self.compartidosCapPal, self.maxRecorte, self.pathLogLogistica, self.env)

        for h in infdem:
            infdem[h] = sum(infdemg[hh,g] for (hh,g) in infdemg if hh==h)    

        #
        #    Se debe permitir cuadrar en demanda que fue recortada.   Para eso se calcula la cantidad recortada
        #    para cada j,h  
        #    se le podria restar la cantidad recortada que fue demanda insatisfecha por frescura, pero no sabemos
        #    en cual sucursal fue descontada esa demanda. (las restricciones de frescura en el prob de cuadratura 
        #    ya harian eso)
            
        rec4quad = {}
        for j,h in self.maxRecorte:
            rec4quad[j,h] = sum(dfxfull[h,g,j]-dfx[h,g,j] for g in SETD if (h,g,j) in dfx)
            
        if min(rec4quad[j,h] for (j,h) in rec4quad) < 0:
        #    print('Error: recorte de h=', h,' en sucursal j=', h, ' es negativo')            
            print('Error: recorte para algun (j,h) es negativo')            


        for (j,h) in rec4quad:
            if rec4quad[j,h]>0 and (j,h) not in self.JHSET:       # hay que permitir cuadrar en estos (j,h)
                self.JHSET[j,h] = 0
                
        #   check que rec4quad este definido para todo JHSET
        for (j,h) in self.JHSET:
            if (j,h) not in rec4quad:
                rec4quad[j,h] = 0

        #oF = {}
        #for i in I:
        #    for f in F:
        #        oF[i,f] = sum(ofx[h,ds,i] for (h,ds,i) in ofx if h in w[f])
        #




            
        # MODELO AGREGADO
        if self.env == None:
            m = Model('ppl')
        else:
            m = Model('ppl', env=self.env)
        m.setParam('LogFile', self.pathLogLogistica + '/Lognobaja.log')

        # VARIABLES

        #
        #  Solo defino Ac ,  x, z, p, y definidos sobre el mismo grafo
        #  demanda es sobre J y oferta sobre I
        #
        #   necesito arcos para x (indexados en h,d, con i,j in arcon y arcij con j in JuJS)
        #   y arcos para z,p,y   (indexados en i,j in arcon y arcij, k, tambien f para z,p)
        #   necesito arcos para x2 (indexados en h,d, con l,i,j in "arcreals" con j in JuJS)
        #   y arcos para z2,p2,y2   (indexados en l,i,j in "arcreals", k, tambien f para z,p)



        arcsxd ={}                      # indices para x en arcos ij
        for h in H:
            for ds in range(limd[h][0],limd[h][1]+1):
                for i,j in arcij:
                    if (h,ds,j) in dfx:
    #                if (h,ds,j) in dfx and dfx[h,ds,j]>0:
                        for dds in range(ds,limd[h][1]+1):
                            arcsxd[h,dds,i,j] = None
                    jjs = str(j)+'S'
                    if (h,ds,jjs) in dfx:
    #                if (h,ds,jjs) in dfx and dfx[h,ds,jjs]>0:
                         for dds in range(ds,limd[h][1]+1):
                            arcsxd[h,dds,i,jjs] = None
                    
        arcsx2d = {}                    # indices para arcos de viajes directos
        arcsxrd = {}                    #  indices para arcos de consolidacion separando por destino final
        for h in H:
            for g in range(limd[h][0],limd[h][1]+1):
                for (l,i,j) in acdir:
                    if (h,g,I0[l]) in ofx and ofx[h,g,I0[l]]>0:
                        if (h,g,j) in dfx:
    #                    if (h,g,j) in dfx and dfx[h,g,j]>0:
                            if i in cons:
                                for gs in range(g,limd[h][1]+1):
                                    arcsx2d[h,gs,l,i,j] = None
                                    arcsxrd[h,gs,l,i,j] = None
                            else:
                                for gs in range(g,limd[h][1]+1):
                                    arcsx2d[h,gs,l,i,j] = None

                        jjs = str(j)+'S'
                        if (h,g,jjs) in dfx:
    #                    if (h,g,jjs) in dfx and dfx[h,g,jjs]>0:
                            if i in cons:
                                for gs in range(g,limd[h][1]+1):
                                    arcsx2d[h,gs,l,i,jjs] = None
                                    arcsxrd[h,gs,l,i,jjs] = None
                            else:
                                for gs in range(g,limd[h][1]+1):
                                    arcsx2d[h,gs,l,i,jjs] = None


    ###
    #      Eliminar productos calzados de viajes de consolidacion

        arcscalzado = dict(arcsxrd)                 
        for (h,g,l,i,j) in arcscalzado:
            if (h,j) in self.calzados:
                arcsxrd.pop((h,g,l,i,j))
            if (h,str(j)[:-1]) in self.calzados:
                arcsxrd.pop((h,g,l,i,str(j)[:-1])) 
            try:
                if h in self.calzados2:
                    arcsxrd.pop((h,g,l,i,j)) 
            except:
                pass
     
        arcsxsd = {}         # indices para variables cajas en viajes compartidos (h,g,j1/j2,i,j1,j2)
                             # i,j1,j2 indican el camion,   j1/j2 indica destino de cargas
        for h in H:
            for g in range(limd[h][0],limd[h][1]+1):
                for (i,j1,j2) in arcijj:
                    if (h,g,I0[i]) in ofx and ofx[h,g,I0[i]]>0:
                        j1s = str(j1)+'S'
                        j2s = str(j2)+'S'
                        if (h,g,j1) in dfx:
    #                    if (h,g,j1) in dfx and dfx[h,g,j1]>0:
                            for gs in range(g,limd[h][1]+1):
                                arcsxsd[h,gs,j1,i,j1,j2] = None
                        if (h,g,j2) in dfx:
    #                    if (h,g,j2) in dfx and dfx[h,g,j2]>0:
                            for gs in range(g,limd[h][1]+1):
                                arcsxsd[h,gs,j2,i,j1,j2] = None
                        if (h,g,j1s) in dfx:
    #                    if (h,g,j1s) in dfx and dfx[h,g,j1s]>0:
                            for gs in range(g,limd[h][1]+1):
                                arcsxsd[h,gs,j1s,i,j1,j2] = None
                        if (h,g,j2s) in dfx:
    #                    if (h,g,j2s) in dfx and dfx[h,g,j2s]>0:
                            for gs in range(g,limd[h][1]+1):
                                arcsxsd[h,gs,j2s,i,j1,j2] = None
           
        x = {}
        for h,g,i,j in arcsxd:
            x[h,g,i,j] = m.addVar(vtype="C",lb=0,name='x_%s_%s_%s_%s' % (h, g, i, j))

        x2 = {}
        for h,g,l,i,j in arcsx2d:
            x2[h,g,l,i,j] = m.addVar(vtype="C",lb=0,name='x2_%s_%s_%s_%s_%s' % (h, g, l, i, j))

        xr = {}
        for h,g,l,i,j in arcsxrd:
            xr[h,g,l,i,j] = m.addVar(vtype="C",lb=0,name='xr_%s_%s_%s_%s_%s' % (h, g, l, i, j))
         
        xs = {}
        for h,g,jd,i,j1,j2 in arcsxsd:
            xs[h,g,jd,i,j1,j2] = m.addVar(vtype="C",lb=0,name='xs_%s_%s_%s_%s_%s_%s' % (h, g, jd, i, j1, j2))
              
        p = {} 
        for i,j in arcij:
            for f in F:
                p[i,j,f] = m.addVar(vtype="C",lb=0,name='p_%s_%s_%s' % (i, j, f))   

        pr = {} 
        for l,i,j in acdir:
            if i in cons:
                for f in F:
                    pr[l,i,j,f] = m.addVar(vtype="C",lb=0,name='pr_%s_%s_%s_%s' % (l, i, j, f))   

        p2 = {} 
        for l,i,j in acdir:
            for f in F:
                p2[l,i,j,f] = m.addVar(vtype="C",lb=0,name='p2_%s_%s_%s_%s' % (l, i, j, f))   

        ps = {} 
        for i,j1,j2 in arcijj:
            for f in F:
                ps[i,j1,j2,f] = m.addVar(vtype="C",lb=0,name='ps_%s_%s_%s_%s' % (i, j1, j2, f))   

        z = {} 
        for i,j in arcij:
            for f in F:
                z[i,j,f] = m.addVar(vtype="I",lb=0,name='z_%s_%s_%s' % (i, j, f))   

        zr = {} 
        for l,i,j in acdir:
            if i in cons:
                for f in F:
                    zr[l,i,j,f] = m.addVar(vtype="I",lb=0,name='zr_%s_%s_%s_%s' % (l, i, j, f))   

        z2 = {} 
        for l,i,j in acdir:
            for f in F:
                z2[l,i,j,f] = m.addVar(vtype="I",lb=0,name='z2_%s_%s_%s_%s' % (l, i, j, f))   

        zs = {} 
        for i,j1,j2 in arcijj:
            for jd in {j1,j2}:
                for f in F:
                    zs[jd,i,j1,j2,f] = m.addVar(vtype="I",lb=0,name='zs_%s_%s_%s_%s_%s' % (jd, i, j1, j2, f))   

        zc = {} 
        for i,j in arcij:
            zc[i,j] = m.addVar(vtype="I",lb=0,name='zc_%s_%s' % (i, j))   

        zrc = {} 
        for l,i,j in acdir:
            if i in cons:
                zrc[l,i,j] = m.addVar(vtype="I",lb=0,name='zrc_%s_%s_%s' % (l, i, j))
                
        z2c = {} 
        for l,i,j in acdir:
            z2c[l,i,j] = m.addVar(vtype="I",lb=0,name='z2c_%s_%s_%s' % (l, i, j))   
           
        zsc = {} 
        for i,j1,j2 in arcijj:
            zsc[i,j1,j2] = m.addVar(vtype="I",lb=0,name='zsc_%s_%s_%s' % (i, j1, j2))   
            
        zMc = {}
        zNc = {}
        zIc = {}
        zFc = {}
        for i,j in arcij:
            zMc[i,j] = m.addVar(vtype="I",lb=0,name='zMc_%s_%s' % (i, j))   
            zNc[i,j] = m.addVar(vtype="I",lb=0,name='zNc_%s_%s' % (i, j))   
            zIc[i,j] = m.addVar(vtype="B",lb=0,name='zIc_%s_%s' % (i, j))   
            zFc[i,j] = m.addVar(vtype="B",lb=0,name='zFc_%s_%s' % (i, j))   

        zMrc = {}
        zNrc = {}
        zIrc = {}
        zFrc = {}
        for l,i in arcon:
            zMrc[l,i] = m.addVar(vtype="I",lb=0,name='zMrc_%s_%s' % (l, i))
            zNrc[l,i] = m.addVar(vtype="I",lb=0,name='zNrc_%s_%s' % (l, i))
            zIrc[l,i] = m.addVar(vtype="B",lb=0,name='zIrc_%s_%s' % (l, i))
            zFrc[l,i] = m.addVar(vtype="B",lb=0,name='zFrc_%s_%s' % (l, i))

        zM2c = {} 
        zN2c = {} 
        zI2c = {} 
        zF2c = {}
        for l,i,j in acdir:
            zM2c[l,i,j] = m.addVar(vtype="I",lb=0,name='zM2c_%s_%s_%s' % (l, i, j))   
            zN2c[l,i,j] = m.addVar(vtype="I",lb=0,name='zN2c_%s_%s_%s' % (l, i, j))   
            zI2c[l,i,j] = m.addVar(vtype="B",lb=0,name='zI2c_%s_%s_%s' % (l, i, j))   
            zF2c[l,i,j] = m.addVar(vtype="B",lb=0,name='zF2c_%s_%s_%s' % (l, i, j))   

        zMsc = {} 
        zNsc = {} 
        zIsc = {} 
        zFsc = {}
        for i,j1,j2 in arcijj:
            zMsc[i,j1,j2] = m.addVar(vtype="I",lb=0,name='zMsc_%s_%s_%s' % (i, j1, j2))   
            zNsc[i,j1,j2] = m.addVar(vtype="I",lb=0,name='zNsc_%s_%s_%s' % (i, j1, j2))   
            zIsc[i,j1,j2] = m.addVar(vtype="B",lb=0,name='zIsc_%s_%s_%s' % (i, j1, j2))   
            zFsc[i,j1,j2] = m.addVar(vtype="B",lb=0,name='zFsc_%s_%s_%s' % (i, j1, j2))   
            
        y = {}
        for (i,j,k) in ARCVK:
            y[i,j,k] = m.addVar(vtype="I",lb=0,name='y_%s_%s_%s' % (i, j, k))   

        y2 = {}
        for (l,i,j,k) in ARCV2K:
            y2[l,i,j,k] = m.addVar(vtype="I",lb=0,name='y2_%s_%s_%s_%s' % (l, i, j, k))   
            
        ys = {}
        for (i,j1,j2,k) in ARCVSK:
            ys[i,j1,j2,k] = m.addVar(vtype="I",lb=0,name='ys_%s_%s_%s_%s' % (i, j1, j2, k))   
            
        o1 = {}
        o2 = {}
        for (h,g,i) in ofx:
            o1[h,g,i] = m.addVar(vtype="C",lb=0,name='o1_%s_%s_%s' % (h,g,i))
            o2[h,g,i] = m.addVar(vtype="C",lb=0,name='o2_%s_%s_%s' % (h,g,i))
            
        u = {}
        for (h,g,j) in dfx:
            u[h,g,j] = m.addVar(vtype="C",lb=0,name='u_%s_%s_%s' % (h,g,j))


        print('Se definieron las variables ')    
        #RESTRICCIONES

        ##Capacidades caminones en kg y pallets   
        #
        for i,j in arcij:
            if i in cons:
                m.addConstr(quicksum(p[i,j,f] for f in F) + quicksum(p2[I1[l],i,j,f] for f in F for l in I1 if (I1[l],i,j) in acdir) + quicksum(pr[I1[l],i,j,f] for f in F for l in I1 if (I1[l],i,j) in acdir) <= quicksum(kg[k]*y[i,j,k] for k in K if (i,j,k) in ARCVK) + quicksum(kg[k]*y2[I1[l],i,j,k] for k in K for l in I1 if (I1[l],i,j,k) in ARCV2K))
                m.addConstr(quicksum(z[i,j,f] for f in fresco) + quicksum(z2[I1[l],i,j,f] for f in fresco for l in I1 if (I1[l],i,j) in acdir) + quicksum(zr[I1[l],i,j,f] for f in fresco for l in I1 if (I1[l],i,j) in acdir) + 2*(zMc[i,j]+zNc[i,j]) + quicksum(2*(zM2c[I1[l],i,j]+zN2c[I1[l],i,j]) for l in I1 if (I1[l],i,j) in acdir) <= quicksum(cap[k]*y[i,j,k] for k in K if (i,j,k) in ARCVK) + quicksum(cap[k]*y2[I1[l],i,j,k] for k in K for l in I1 if (I1[l],i,j,k) in ARCV2K))
            else:
                m.addConstr(quicksum(p[i,j,f] for f in F) + quicksum(p2[I1[l],i,j,f] for f in F for l in I1 if (I1[l],i,j) in acdir) <= quicksum(kg[k]*y[i,j,k] for k in K if (i,j,k) in ARCVK) + quicksum(kg[k]*y2[I1[l],i,j,k] for k in K for l in I1 if (I1[l],i,j,k) in ARCV2K))
                m.addConstr(quicksum(z[i,j,f] for f in fresco) + quicksum(z2[I1[l],i,j,f] for f in fresco for l in I1 if (I1[l],i,j) in acdir) + 2*(zMc[i,j]+zNc[i,j]) + quicksum(2*(zM2c[I1[l],i,j]+zN2c[I1[l],i,j]) for l in I1 if (I1[l],i,j) in acdir) <= quicksum(cap[k]*y[i,j,k] for k in K if (i,j,k) in ARCVK) + quicksum(cap[k]*y2[I1[l],i,j,k] for k in K for l in I1 if (I1[l],i,j,k) in ARCV2K))
            m.addConstr(2*(zMc[i,j]+zNc[i,j])   <= quicksum(cap[k]*y[i,j,k] for k in K if (i,j,k) in ARCVK))
                
        for l,i in arcon:
            m.addConstr(quicksum(pr[l,i,j,f] for f in F for j in JuJS if (l,i,j) in acdir) <= quicksum(kg[k]*y[l,i,k] for k in K if (l,i,k) in ARCVK))
            m.addConstr(quicksum(zr[l,i,j,f] for f in fresco for j in JuJS if (l,i,j) in acdir) + 2*(zMrc[l,i]+zNrc[l,i]) <= quicksum(cap[k]*y[l,i,k] for k in K if (l,i,k) in ARCVK))

        for l,i,j in acdir:
            m.addConstr(quicksum(p2[l,i,j,f] for f in F)<=quicksum(kg[k]*y2[l,i,j,k] for k in K if (l,i,j,k) in ARCV2K)) 
            m.addConstr(quicksum(z2[l,i,j,f] for f in fresco) + 2*(zM2c[l,i,j]+zN2c[l,i,j]) <=quicksum(cap[k]*y2[l,i,j,k] for k in K if (l,i,j,k) in ARCV2K)) 

        for i,j1,j2 in arcijj:
            m.addConstr(quicksum(ps[i,j1,j2,f] for f in F)<=quicksum(kg[k]*ys[i,j1,j2,k] for k in K if (i,j1,j2,k) in ARCVSK)) 
            m.addConstr(quicksum(zs[jd,i,j1,j2,f] for jd in {j1,j2} for f in fresco) + 2*(zMsc[i,j1,j2]+zNsc[i,j1,j2]) <=quicksum(cap[k]*ys[i,j1,j2,k] for k in K if (i,j1,j2,k) in ARCVSK)) 


        #
        # Cortes
        for j in J:
            m.addConstr(quicksum(cap[kk]*y[ii,jj,kk] for (ii,jj,kk) in ARCVK if jj==j) + quicksum(cap[kk]*y2[ll,ii,jj,kk] for (ll,ii,jj,kk) in ARCV2K if jj==j) + quicksum(cap[k]*ys[i,j1,j2,k] for (i,j1,j2,k) in ARCVSK if j1==j or j2==j) >= quicksum(dfx[h,ds,j]/cajaspallet[h] for h in H for ds in SETD if (h,ds,j) in dfx) + quicksum(dfx[h,ds,str(j)+'S']/cajaspallet[h] for h in H for ds in SETD if (h,ds,str(j)+'S') in dfx))
            for f in F:
                m.addConstr(quicksum(z[I2[i],j,f] for i in I2 if (I2[i],j) in arcij) + quicksum(z2[I1[l],I2[i],j,f] for l in I1 for i in I2 if (I1[l],I2[i],j) in acdir) + quicksum(zr[I1[l],i,j,f] for l in I1 for i in cons if (I1[l],i,j) in acdir) + quicksum(zs[jd,i,j1,j2,f] for (i,j1,j2) in arcijj for jd in {j1,j2} if j1==j or j2==j)  >= quicksum(dfx[h,ds,j]/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,j) in dfx) + quicksum(dfx[h,ds,str(j)+'S']/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,str(j)+'S') in dfx))
    #   agregar cortes en los z por familia?
    #            

        #m.addConstr(quicksum(y2[l,i,j,k] for (l,i,j,k) in ARCV2K) <= 30 )    

        #Definir p     
        #Definir pallets  z, z2  y pares de pallets congelados zc, z2c
        #
        for i,j in arcij:
            m.addConstr(quicksum(z[i,j,f] for f in congelado)+quicksum(zr[I1[l],i,j,f] for f in congelado for l in I1 if (I1[l],i,j,f) in zr)  <= 2*zc[i,j] )
            for f in list(w.keys()):
                m.addConstr(quicksum(x[h,ds,i,j]*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,i,j) in arcsxd) + quicksum(x[h,ds,i,str(j)+'S']*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,i,str(j)+'S') in arcsxd)<=p[i,j,f])
                m.addConstr(quicksum(x[h,ds,i,j]/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,i,j) in arcsxd) + quicksum(x[h,ds,i,str(j)+'S']/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,i,str(j)+'S') in arcsxd)<= z[i,j,f] )

        for l,i,j in acdir:
            m.addConstr(quicksum(z2[l,i,j,f] for f in congelado) <= 2*z2c[l,i,j] )
            if i in cons:
                m.addConstr(quicksum(zr[l,i,j,f] for f in congelado) <= 2*zrc[l,i,j] )
            for f in list(w.keys()):
                m.addConstr(quicksum(x2[h,ds,l,i,j]*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,l,i,j) in arcsx2d) + quicksum(x2[h,ds,l,i,str(j)+'S']*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,l,i,str(j)+'S') in arcsx2d)<=p2[l,i,j,f])
                m.addConstr(quicksum((x2[h,ds,l,i,j]/cajaspallet[h]) for h in w[f] for ds in SETD if (h,ds,l,i,j) in arcsx2d) + quicksum((x2[h,ds,l,i,str(j)+'S']/cajaspallet[h]) for h in w[f] for ds in SETD if (h,ds,l,i,str(j)+'S') in arcsx2d)<= z2[l,i,j,f] )
                if i in cons:
                    m.addConstr(quicksum(xr[h,g,l,i,j]*pesocaja[h] for h in w[f] for g in SETD if (h,g,l,i,j) in arcsxrd) + quicksum(xr[h,g,l,i,str(j)+'S']*pesocaja[h] for h in w[f] for g in SETD if (h,g,l,i,str(j)+'S') in arcsxrd)<=pr[l,i,j,f])
                    m.addConstr(quicksum(xr[h,g,l,i,j]/cajaspallet[h] for h in w[f] for g in SETD if (h,g,l,i,j) in arcsxrd) + quicksum(xr[h,g,l,i,str(j)+'S']/cajaspallet[h] for h in w[f] for g in SETD if (h,g,l,i,str(j)+'S') in arcsxrd)<=zr[l,i,j,f])

        for i,j1,j2 in arcijj:
            m.addConstr(quicksum(zs[jd,i,j1,j2,f] for jd in {j1,j2} for f in congelado) <= 2*zsc[i,j1,j2] )
            for f in list(w.keys()):
                m.addConstr(quicksum(xs[h,g,jd,i,j1,j2]*pesocaja[h] for h in w[f] for g in SETD for jd in {j1, j2, str(j1)+'S', str(j2)+'S'} if (h,g,jd,i,j1,j2) in arcsxsd) <=ps[i,j1,j2,f])
                m.addConstr(quicksum((xs[h,g,jd,i,j1,j2]/cajaspallet[h]) for h in w[f] for g in SETD for jd in {j1, str(j1)+'S'} if (h,g,jd,i,j1,j2) in arcsxsd) <= zs[j1,i,j1,j2,f] )
                m.addConstr(quicksum((xs[h,g,jd,i,j1,j2]/cajaspallet[h]) for h in w[f] for g in SETD for jd in {j2, str(j2)+'S'} if (h,g,jd,i,j1,j2) in arcsxsd) <= zs[j2,i,j1,j2,f] )
                
        #  restricciones en congelados si camion es '26P'  
        #
        #   big M se define como 1+ 2 * maximo numero de camiones a j.   En restricciones hay que mult por pallets 
        kM = '26P'
        CAPCAM = cap[kM]
        LIMINFCAP = mincap[kM]
        DELTACAP = CAPCAM - maxcap[kM]
        MM = 1+2*max(math.ceil(sum(dfx[h,g,j]/cajaspallet[h] for (h,g,j) in dfx if j==jj or j==str(jj)+'S')/CAPCAM) for jj in J)

        for l,i in arcon:
            m.addConstr(quicksum(zrc[l,i,j] for j in JuJS if (l,i,j) in acdir) <= zMrc[l,i]+zNrc[l,i])
            m.addConstr(zMrc[l,i] <= CAPCAM*MM*zIrc[l,i])
            m.addConstr(zNrc[l,i] <= CAPCAM*MM*(1-zIrc[l,i]) )
            if (l,i,kM) in ARCVK:
                m.addConstr(2*zMrc[l,i] >= LIMINFCAP*zIrc[l,i])
                m.addConstr(2*zMrc[l,i] >= CAPCAM*y[l,i,kM] - CAPCAM*MM*(1-zFrc[l,i]))
                m.addConstr(2*zMrc[l,i] <= CAPCAM*y[l,i,kM] - DELTACAP*(1-zFrc[l,i]))
    #            m.addConstr(zFrc[l,i] <= zIrc[l,i])
            m.addConstr(2*zNrc[l,i] <= quicksum(cap[k]*y[l,i,k] for k in K if (l,i,k) in ARCVK and k!= kM))

        for l,i,j in acdir:
            m.addConstr(z2c[l,i,j] <= zM2c[l,i,j]+zN2c[l,i,j])
            m.addConstr(zM2c[l,i,j] <= CAPCAM*MM*zI2c[l,i,j])
            m.addConstr(zN2c[l,i,j] <= CAPCAM*MM*(1-zI2c[l,i,j]))
            if (l,i,j,kM) in ARCV2K:
                m.addConstr(2*zM2c[l,i,j] >= LIMINFCAP*zI2c[l,i,j])
                m.addConstr(2*zM2c[l,i,j] >= CAPCAM*y2[l,i,j,kM] - CAPCAM*MM*(1-zF2c[l,i,j]))
                m.addConstr(2*zM2c[l,i,j] <= CAPCAM*y2[l,i,j,kM] - DELTACAP*(1-zF2c[l,i,j]))
    #            m.addConstr(zF2c[l,i,j] <= zI2c[l,i,j])
            m.addConstr(2*zN2c[l,i,j] <= quicksum(cap[k]*y2[l,i,j,k] for k in K if (l,i,j,k) in ARCV2K and k!= kM))

        for i,j in arcij:
            m.addConstr(zc[i,j] <= zMc[i,j]+zNc[i,j]) 
            m.addConstr(zMc[i,j] <= CAPCAM*MM*zIc[i,j])
            m.addConstr(zNc[i,j] <= CAPCAM*MM*(1-zIc[i,j]))
            if (i,j,kM) in ARCVK:
                m.addConstr(2*zMc[i,j] >=  LIMINFCAP*zIc[i,j])
                m.addConstr(2*zMc[i,j] >= CAPCAM*y[i,j,kM] - CAPCAM*MM*(1-zFc[i,j]))
                m.addConstr(2*zMc[i,j] <= CAPCAM*y[i,j,kM]  - DELTACAP*(1-zFc[i,j]))
    #            m.addConstr(2*zMc[i,j] >= CAPCAM*(y[i,j,kM]+quicksum(y2[I1[l],i,j,kM] for l in I1 if (I1[l],i,j,kM) in ARCV2K)) - CAPCAM*MM*(1-zFc[i,j]))
    #            m.addConstr(2*zMc[i,j] <= CAPCAM*(y[i,j,kM]+quicksum(y2[I1[l],i,j,kM] for l in I1 if (I1[l],i,j,kM) in ARCV2K))  - DELTACAP*(1-zFc[i,j]))
     #           m.addConstr(zFc[i,j] <= zIc[i,j])
            m.addConstr(2*zNc[i,j] <= quicksum(cap[k]*y[i,j,k] for k in K if (i,j,k) in ARCVK and k!=kM) )

        for i,j1,j2 in arcijj:
            m.addConstr(zsc[i,j1,j2] <= zMsc[i,j1,j2]+zNsc[i,j1,j2])
            m.addConstr(zMsc[i,j1,j2] <= CAPCAM*MM*zIsc[i,j1,j2])
            m.addConstr(zNsc[i,j1,j2] <= CAPCAM*MM*(1-zIsc[i,j1,j2]))
            if (i,j1,j2,kM) in ARCVSK:
                m.addConstr(2*zMsc[i,j1,j2] >= LIMINFCAP*zIsc[i,j1,j2])
                m.addConstr(2*zMsc[i,j1,j2] >= CAPCAM*ys[i,j1,j2,kM] - CAPCAM*MM*(1-zFsc[i,j1,j2]))
                m.addConstr(2*zMsc[i,j1,j2] <= CAPCAM*ys[i,j1,j2,kM] - DELTACAP*(1-zFsc[i,j1,j2]))
    #            m.addConstr(zFsc[l,i,j] <= zIsc[l,i,j])
            m.addConstr(2*zNsc[i,j1,j2] <= quicksum(cap[k]*ys[i,j1,j2,k] for k in K if (i,j1,j2,k) in ARCVSK and k!= kM))

        #Oferta 
        for (h,g,i) in ofx:
            m.addConstr(o1[h,g,i]+o2[h,g,i] <= ofx[h,g,i])
            m.addConstr( quicksum(xr[h,g,I1[i],I2[l],j] for l in I for j in JuJS if (h,g,I1[i],I2[l],j) in arcsxrd) + quicksum(x2[h,g,I1[i],I2[l],j] for l in I for j in JuJS if (h,g,I1[i],I2[l],j) in arcsx2d) <= o1[h,g,i] )
            m.addConstr( quicksum(x[h,g,I2[i],j] for j in JuJS if (h,g,I2[i],j) in arcsxd) + quicksum(xs[h,g,jd,I2[i],j1,j2] for j1 in JuJS for j2 in JuJS for jd in {j1,j2,str(j1)+'S',str(j2)+'S'} if (h,g,jd,I2[i],j1,j2) in arcsxsd) <= o2[h,g,i] )
            
        #Demanda 
        #          
        for (h,g,j) in dfx:
            m.addConstr(quicksum(x[h,gs,I2[i],j] for i in I for gs in SETD if (h,gs,I2[i],j) in arcsxd and gs >= g) + quicksum(x2[h,gs,I1[l],I2[i],j] for l in I for i in I for gs in SETD if (h,gs,I1[l],I2[i],j) in arcsx2d and gs >= g) + quicksum(xr[h,gs,I1[l],I2[i],j] for l in I for i in I for gs in SETD if (h,gs,I1[l],I2[i],j) in arcsxrd and gs >= g )  + quicksum(xs[hh,gs,jd,i,j1,j2] for (hh,gs,jd,i,j1,j2) in arcsxsd if hh==h and jd==j and gs >= g)  + u[h,g,j] == dfx[h,g,j] )  

        #
        #Limit on infeasibility
        for h in H:
            m.addConstr(quicksum(u[h,ds,j] for j in JuJS for ds in SETD if (h,ds,j) in u) <= infdem[h])           
            


        #FO DE CV*Y SOLOS
        m.setObjective(quicksum(cvdatos[i,j,k]*y[i,j,k] for i,j,k in ARCVK) + quicksum(cv2datos[l,i,j,k]*y2[l,i,j,k] for l,i,j,k in ARCV2K) + quicksum(cvsdatos[i,j1,j2,k]*ys[i,j1,j2,k] for i,j1,j2,k in ARCVSK) +quicksum(z[i,j,f]*0.1 for (i,j) in arcij for f in F)+quicksum(zMc[i,j]*0.1+zNc[i,j]*0.2 for (i,j) in arcij) +quicksum(z2[l,i,j,f]*0.1 for (l,i,j) in acdir for f in F)+quicksum(zM2c[l,i,j]*0.1+zN2c[l,i,j]*0.2 for (l,i,j) in acdir)+quicksum(zr[l,i,j,f]*0.1 for (l,i,j) in acdir for f in F if i in cons)+quicksum(zMrc[l,i]*0.1+zNrc[l,i]*0.2 for (l,i) in arcon) +quicksum(zs[jd,i,j1,j2,f]*0.1 for (i,j1,j2) in arcijj for jd in {j1,j2} for f in F)+quicksum(zMsc[i,j1,j2]*0.1+zNsc[i,j1,j2]*0.2 for (i,j1,j2) in arcijj),GRB.MINIMIZE)   



        #
        #   to define a starting solution:
        #
        #for (i,j,f) in solz:
        #    z[i,j,f].start = solz[i,j,f]
        #    



        m.setParam('TimeLimit', self.tiempoLimite)#900)    # 15 minutos
        m.setParam('MIPFocus', 2)
        m.setParam('Cuts', 3)
        m.setParam('GomoryPasses',1)
        m.setParam('StrongCGcuts',2)
        m.setParam('Heuristics',0.5)
        m.setParam('NormAdjust',2)
        m.setParam('PreDual',1)
        m.setParam('PreDepRow',1)

        m.optimize()

        if m.status >= 3 and m.status <=4:
            m.computeIIS()
            m.write(self.pathLogLogistica + '/v13largoINF.ilp')
            raise ValueError("Error en optimización. Modelo de optimización de camiones es infactible.");

        #
        #   GET SOLUTION.   MAKE INTEGER BOXES
        #
        soly = m.getAttr('x', y)
        for (i,j,k) in ARCVK:
            if (i,j,k) not in soly:
                soly[i,j,k] = 0
        soly = {(i,j,k) : round(soly[i,j,k]) for (i,j,k) in soly}

        soly2 = m.getAttr('x', y2)
        soly2 = {(l,i,j,k) : round(soly2[l,i,j,k]) for (l,i,j,k) in soly2}

        solys = m.getAttr('x', ys)
        solys = {(i,j1,j2,k) : round(solys[i,j1,j2,k]) for (i,j1,j2,k) in solys}
         
         
        solx = m.getAttr('x', x)
        solxi = {}
        for (h,g,i,j) in solx:
            if abs(round(solx[h,g,i,j])-solx[h,g,i,j])<1e-5:
                solxi[h,g,i,j] = round(solx[h,g,i,j])
            else:
                solxi[h,g,i,j] = math.floor(solx[h,g,i,j])


        solxr = m.getAttr('x', xr)
        solxri = {}
        for (h,g,l,i,j) in solxr:
            if abs(round(solxr[h,g,l,i,j])-solxr[h,g,l,i,j])<1e-5:
                solxri[h,g,l,i,j] = round(solxr[h,g,l,i,j])
            else:
                solxri[h,g,l,i,j] = math.floor(solxr[h,g,l,i,j])


        solx2 = m.getAttr('x', x2)
        solx2i = {}
        for (h,g,l,i,j) in solx2:
            if abs(round(solx2[h,g,l,i,j])-solx2[h,g,l,i,j])<1e-5:
                solx2i[h,g,l,i,j] = round(solx2[h,g,l,i,j])
            else:
                solx2i[h,g,l,i,j] = math.floor(solx2[h,g,l,i,j])

        solxs = m.getAttr('x', xs)
        solxsi = {}
        for (h,g,jd,i,j1,j2) in solxs:
            if abs(round(solxs[h,g,jd,i,j1,j2])-solxs[h,g,jd,i,j1,j2])<1e-5:
                solxsi[h,g,jd,i,j1,j2] = round(solxs[h,g,jd,i,j1,j2])
            else:
                solxsi[h,g,jd,i,j1,j2] = math.floor(solxs[h,g,jd,i,j1,j2])



        ##########
        #    check if sent correct number of vehicles to distribution centers
        #    with a limit on number of trucks:
        #
        solz = m.getAttr('x',z)
        solz2 = m.getAttr('x',z2)
        solzr = m.getAttr('x',zr)
        logAlertas = open(self.pathLogLogistica + "/problemarecorte.log","a")
        logAlertas.write('############################ \n')
        logAlertas.write('Comparacion solicitud recorte con recorte \n')
        for j in self.compartidosCamiones:
            if self.compartidosCamiones[j]>0:
                if sum(soly[i,jj,k] for (i,jj,k) in soly if jj==j)+sum(soly2[l,i,jj,k] for (l,i,jj,k) in soly2 if jj==j) > self.compartidosCamiones[j]:
                    print('Se piden ',self.compartidosCamiones[j],' en ',j, ' se tienen:')
                    logAlertas.write('Se piden ' + str(self.compartidosCamiones[j]) + ' en ' +str(j)+' se tienen:\n') 
                    for (i,jj,k) in soly:
                        if jj==j and soly[i,jj,k]>0:
                            print(i,jj,k,soly[i,jj,k])
                            logAlertas.write(str(i)+', '+str(jj)+', '+str(k)+', '+str(soly[i,jj,k])+'\n')
                    for (l,i,jj,k) in soly2:
                        if jj==j and soly2[l,i,jj,k]>0:
                            print(l,i,jj,k,soly2[l,i,jj,k])
                            logAlertas.write(str(l)+', '+str(i)+', '+str(jj)+', '+str(k)+', '+str(soly2[l,i,jj,k])+'\n')
                    aux1 = sum(solz[i,jj,f] for (i,jj,f) in solz if jj==j and f in fresco)+sum(solzr[l,i,jj,f] for (l,i,jj,f) in solzr if jj==j and f in fresco)
                    aux2 = sum(solz[i,jj,f] for (i,jj,f) in solz if jj==j and f in congelado)+sum(solzr[l,i,jj,f] for (l,i,jj,f) in solzr if jj==j and f in congelado)
                    aux1 = aux1+sum(solz2[l,i,jj,f] for (l,i,jj,f) in solz2 if jj==j and f in fresco)
                    aux2 = aux2+sum(solz2[l,i,jj,f] for (l,i,jj,f) in solz2 if jj==j and f in congelado)
                    print('Llevando en total pallets ', aux1,' frescos y ', aux2,' congelados')    
                    logAlertas.write('Llevando en total pallets '+str(aux1)+' frescos y '+ str(aux2)+' congelados\n')    
        logAlertas.close()
        
        ##########

        #
        #   PRINT SOLUTION
        #    drop significant fraction of boxes
        #


        var_names = []
        var_values = []

        for var in m.getVars():
            var_names.append(str(var.varName))
            var_values.append(var.X)

        var_vals = []
        for indx in range(len(var_names)):
            ax = var_names[indx].split('_')
            var_vals.append(var_values[indx])
            if ax[0][:1] =='x':
                if abs(round(var_values[indx])-var_values[indx]) < 1e-5:
                    var_vals[indx] = round(var_values[indx])
                else:
                    var_vals[indx] = math.floor(var_values[indx])        
            if ax[0][:1] =='y' or ax[0][:1]=='z':
                var_vals[indx] = round(var_values[indx])

        self.tieneSolucion = len(var_names) > 0
        
        with open(self.pathTmpLogistica + '/testout-v13largo.csv', 'w') as myfile:
            wr = csv.writer(myfile, lineterminator = '\n')
            wr.writerows(zip(var_names, var_vals))


        if self.conCuadratura:
            qm = cuad.cuadratura(self.JHSET, soly, soly2, solys, solxi, solxri, solx2i, solxsi, ofx, SETD, arcij, arcon, acdir, arcijj, arcsxd, arcsx2d, arcsxrd, arcsxsd, ARCVK, ARCV2K, ARCVSK, I, I0, I1, I2, cons, JuJS, J, self.K, self.F, self.w, congelado, fresco, pesocaja, kg, cajaspallet, cap, mincap, maxcap, kM, MM, pre, c, rec4quad, self.pathLogLogistica, self.env)

            var_names = []
            var_values = []

            if qm.status >= 3 and qm.status <=4:
                qm.computeIIS()
                qm.write(self.pathTmp + '/cuadratINF.ilp')
                for var in qm.getVars():
                    var_names.append(str(var.varName))
                    var_values.append(0)
                raise ValueError("Error en optimización. Modelo de cuadratura es infactible.")
            else:
                for var in qm.getVars():
                    var_names.append(str(var.varName))
                    var_values.append(var.X)

            var_vals = []
            for indx in range(len(var_names)):
                ax = var_names[indx].split('_')
                var_vals.append(var_values[indx])
                if ax[0][:2] =='qx':
                    if abs(round(var_values[indx])-var_values[indx]) < 1e-5:
                        var_vals[indx] = round(var_values[indx])
                    else:
                        var_vals[indx] = math.floor(var_values[indx])
                if ax[0][:2]=='qz':
                    var_vals[indx] = round(var_values[indx])

            #
            #
            with open(self.pathTmpLogistica + '/testout-v13cuad.csv', 'w') as myfile:
                wr = csv.writer(myfile, lineterminator = '\n')
                wr.writerows(zip(var_names, var_vals))




        #
        #
        #   summary 
        #
        #print('Total routes planta-sucursal in solution y ', sum(soly[i,j,k] for (i,j,k) in soly if i in I2.values()))
        #print('Total routes planta-planta in solution y ', sum(soly[i,j,k] for (i,j,k) in soly if j in cons))
        #print('Total routes planta-planta-sucursal in solution y2 ', sum(soly2[l,i,j,k] for (l,i,j,k) in soly2))
        #
        #print('La cantidad total demandada (y lo disponible en stock) en cajas es')
        #print('demanda : ',sum(dfx[h,g,j] for h,g,j in dfx) )
        #print('oferta : ', sum(ofx[h,g,i] for h,g,i in ofx) )
        #
        #print('En cajas')
        #print('El total transportado directo planta-sucursal ', sum(solx[h,g,i,j] for (h,g,i,j) in solx if i in I2.values()))    
        #print('El total consolidados y luego planta-sucursal ', sum(solxr[h,g,l,i,j] for (h,g,l,i,j) in solxr))    
        #print('El total enviado en viaje planta-planta-sucursal ', sum(solx2[h,g,l,i,j] for (h,g,l,i,j) in solx2))    
            
        #print('En camiones cajas/pallets/26')
        #print('El total transportado directo planta-sucursal ', sum(solx[h,g,i,j]/cajaspallet[h]/26 for (h,g,i,j) in solx if i in I2.values()))    
        #print('El total consolidados y luego planta-sucursal ', sum(solxr[h,g,l,i,j]/cajaspallet[h]/26 for (h,g,l,i,j) in solxr))    
        #print('El total enviado en viaje planta-planta-sucursal ', sum(solx2[h,g,l,i,j]/cajaspallet[h]/26 for (h,g,l,i,j) in solx2))    
        #
        #
        #print('Respuesta sol sat demanda')
        #for i in I:
        #    print('Total de cj desde planta ', i,' = ', sum(solx[h,g,ii,j] for (h,g,ii,j) in solx if I0[ii]==i)+sum(solxr[h,g,ii,ll,j] for (h,g,ii,ll,j) in solxr if I0[ii]==i)+sum(solx2[h,g,ii,ll,j] for (h,g,ii,ll,j) in solx2 if I0[ii]==i))
            

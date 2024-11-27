# -*- coding: utf-8 -*-
"""
Created on Tue Jul  7 20:47:39 2020

@author: fernando
"""


#
#
# Ejemplo
#d = {(1012196, 'T012'): 8,
# (1012196, 'T014'): 20,
# (1012196, 'T015'): 38,
# (1012196, 'T017'): 58,
# (1012196, 'T021'): 12,
# (1012196, 'T022'): 7,
# (1012196, 'T052'): 8,
# (1012196, 'T053'): 4,
# (1012196, 'T061'): 4,
# (1020356, 'T017'): 58,
# (1020356, 'T021'): 12,
# (1020356, 'T022'): 7,
# (1020356, 'T055'): 8,
# (1020356, 'T063'): 4, }
#
#SL = {}
#SL[1012196] = 12
#
#H = {1012196, 1020356}
#
#J = {'T012','T014','T015','T017','T021','T022','T052','T053','T061'}
#
#I = {'P001', 'P002'}
#
#LT = {}
#SL = {}
#for h in H:
#    SL[h]=3
#    for j in J:
#       if j == 'T021' or j=='T022':
#            LT[h,j]=2
#        else:
#            LT[h,j]=1
#
#LT[1012196,'T014'] = 0
#LT[1012196,'T015'] = 4
#LT[1012196,'T017'] = 6
#
#
#ostock={}
#ostock[1012196,7,'P001'] = 61
#ostock[1012196,8,'P002'] = 62
#ostock[1012196,8,'P001'] = 63
#ostock[1012196,6,'P002'] = 64
#ostock[1020356,6,'P001'] = 65
#ostock[1020356,5,'P002'] = 66
#




# Listado de fechas limite en sucursales para producto h   
# dds[dia]  dias limites para demanda 
# defino dstock, demanda en dia dds en sucursal j (de producto h)
# se construye al recorrer la demanda por producto y sucursales
#
#  Necesita:
#  I conjunto de plantas
#  J conjunto de sucursales con supermercados aparte 
#  dori[h,j] como antes, demanda supermercados en nodo supermercado
#  LT[h,j]  por sucursal (y producto por culpa de los supermercados incluye tiempo de viaje + requisito)
#  SL[h]  de maestro materiales shelf life se ocupa para definir ostock
#  ostock[h,d,i]  oferta en planta i de producto h con d dias mas de vida
#

def frescura(H, JuJS, dori, LT, ostock, I, pathLog):
    dstock = {}
    dfx = {}
    ofx = {}
    limd = {}
    for h in H:
        dds = {}
        for j in JuJS:
            if (h,j) in dori and dori[h,j]>=0:
                dds[LT[h,j]] = None
                dstock[h,LT[h,j],j]=dori[h,j]
                
        
    # supongo que tengo leido el archivo stock almacenado en 
    # ostock[h,ds,i]  oferta de producto h, con ds dias de vida, en planta i
    #
        ods = {}
        for (hh,ds,i) in ostock:
            if hh==h:
                ods[ds] = None
        
        sdds = sorted(dds)
        sods = sorted(ods)
        jds = 0
        ids = 0
        indx = 0
        limi = 0
        valid = True
        while ids < len(sods) and jds < len(sdds):
            if sods[ids]>=sdds[jds]:
                if not valid:
                    indx += 1
                valid = True
                for j in JuJS:
                    if (h,sdds[jds],j) in dstock:
                        if (h,indx,j) in dfx:
                            dfx[h,indx,j] += dstock[h,sdds[jds],j]
                        else:
                            dfx[h,indx,j] = dstock[h,sdds[jds],j]
                jds += 1
            else:
                if ids==0 and jds==0:
                    limi=1
                valid = False
                for i in I:
                    if (h,sods[ids],i) in ostock:
                        if (h,indx,i) in ofx:
                            ofx[h,indx,i] += ostock[h,sods[ids],i]
                        else:
                            ofx[h,indx,i] = ostock[h,sods[ids],i]
                ids += 1

        limd[h] = [limi, indx]     
        #  limites en g entre los que hay flujo.  
        # solo definimos variables de flujo con g in limd[h].  
        # Ojo que dfx y ofx pueden estar definidos afuera de limd[h] pero no los dos
        # si limd[h] = [1 0]  no hay flujo factible.

        for ii in range(ids,len(sods)):
            for i in I:
                if (h,sods[ii],i) in ostock:
                    if (h,indx,i) in ofx:
                        ofx[h,indx,i] += ostock[h,sods[ii],i]
                    else:
                        ofx[h,indx,i] = ostock[h,sods[ii],i]

        for ii in range(jds,len(sdds)):
            indx +=1
            for j in JuJS:
                if (h,sdds[ii],j) in dstock:
                    dfx[h,indx,j] = dstock[h,sdds[ii],j]

    #
    #
    #definir demanda insatisfecha  infdem[h]  demanda no cubierta considerando frescura
    #
    #
    #  deficit acumula lo que falta para cada h y g.  (oferta -demanda)_- 
    #  demanda total insatisfecha es el deficit en el g mas grande (mas nuevo)+ofertas mas nuevas
    #
    #  Observacion:  si deficit[h,g]==0 entonces no infdemg[h,gs]==0 si gs<= g
    #   similarmente, si infdemg[h,g]>0  entonces deficit[h,gs]<0 para gs> g
    #   esto significa
    infdem = {}
    deficit = {}
    for h in H:
        indg = 0
        maxg = max(g for (hh,g,j) in dfx if hh==h)
        while indg <= maxg:
            aux = sum(ofx[hh,gg,ii] for (hh,gg,ii) in ofx if hh==h and gg==indg)-sum(dfx[hh,gg,jj] for (hh,gg,jj) in dfx if hh==h and gg==indg)
            if indg > 0:
                aux += deficit[h,indg-1]
            if aux >= 0:               #  exceso de material, no hay dem insat con g menor
                deficit[h,indg] = 0
                for gg in range(indg):
                    deficit[h,gg] = 0
            else: 
                deficit[h,indg] = aux
            indg += 1
        if maxg > -1:
            infdem[h] = -1*(deficit[h,maxg] + sum(ofx[hh,gg,ii] for (hh,gg,ii) in ofx if hh==h and gg>maxg))

#   demanda infactible segun frescura g.   Se prefiere satisfacer demandas que tienen g
#   mas altos, quiere decir estan mas lejos. 
#
    infdemg = {}
    for h in H:
        maxg = max(g for (hh,g,j) in dfx if hh==h)       # maxg = max((g for (hh,g) in deficit if hh==h),default=-1)
        auxp = 0
        for ind in range(maxg,0,-1):
            aux = deficit[h,ind]-deficit[h,ind-1]+auxp
            if aux>0:
                infdemg[h,ind] = 0
                auxp = aux
            else:
                infdemg[h,ind] = -1*aux
                auxp = 0
        if maxg > -1:
            infdemg[h,0] = -1*deficit[h,0] + auxp           

            
    logAlertas = open(pathLog + "/unmet-frescura.log","w")
    for h in infdem:
        if infdem[h] > 0:
            print('unmet demand ', h, infdem[h])
            logAlertas.write('unmet demand ' + str(h) + ", " + str(infdem[h]) + "\n") 
#            for (hh,g) in deficit:
#                if hh==h: 
#                    if g==0:
#                        print('unmet demand ', h, g, deficit[h,g])
#                        logAlertas.write('unmet demand ' + str(h) + ", " + str(g) + ", " + str(deficit[h,g]) + "\n") 
#                    else:
#                        print('unmet demand ', h, g, deficit[h,g-1]-deficit[h,g])
#                        logAlertas.write('unmet demand ' + str(h) + ", " + str(g) + ", " + str(deficit[h,g-1]-deficit[h,g]) + "\n") 

    logAlertas.close()
        
    return dfx, ofx, limd, infdem, infdemg
    
#  to debug: 
#ff = algun h
#for (h,ds,i) in ostock:
#    if h==ff and ostock[h,ds,i]>0:
#        print(h,ds,i,ostock[h,ds,i])
#        
#for (h,ds,i) in dstock:
#    if h==ff and dstock[h,ds,i]>0:
#        print(h,ds,i,dstock[h,ds,i])
#
#daux = {}        
#for (h,ds,i) in dstock:
#    if h==ff:
#        daux[ds] = sum(dstock[hj,dsj,ij] for (hj,dsj,ij) in dstock if hj==ff and dsj==ds)
#for ds in daux:
#    if daux[ds]>0:
#        print(h,ds,daux[ds])
#        
        
#                
#
#   Termina con ofx y dfx que tienen la oferta y demanda respectivamente
#   agrupada en conjuntos de frescura.
#
#    dfx[h,d,j] es satisfecho por sum(ofx[hh,dd,ii] for (hh,dd,ii) in ofx if hh==h and dd >= d)
#
#    si para algun producto h,  dfx[h,d,j] no tiene grupo d==0, entonces hay producto 
#    basura en ofx[h,0,i]
#
#    es decir:   
#    basura=True
#    for (h,d,i) in dfx:
#       if d == 0:
#           basura=False
            
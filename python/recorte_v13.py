# -*- coding: utf-8 -*-
"""
Created on Wed May 19 01:27:53 2021

@author: fernando
"""

#
#  problema RECORTE
#
#  input  
#    citypair     #  tabla nx2  sucursales compartidas por fila
#    pareadas     #  listado de sucursales que pueden tener una ruta compartida
#    compartidosCamiones = {}
#    maxRecorte = {}
#    dfx
#    J
#    pesocaja[h]
#    cajaspallet[hh]
#    (pre[j,h]-c[h])
#    infdemg
#
#    
#  output
#    modificacion a dfx reduciendo la demanda para satisfacer recortes en camiones 
#    a sucursales
#

from gurobipy import *
from gurobipy import GRB
from gurobipy import Model
from gurobipy import quicksum
#import math 


def recorte(J, dfx, pesocaja, cajaspallet, pre, c, infdemg, citypair, pareadas, compartidosCamiones, compartidosCapPal, maxRecorte, pathLog, env):

    
#   compartidosCapPal[j]   tiene la capacidad en pallets de los camiones limitados. Default 24 o se ingreso algo
    capKGStandard = 25000
    

#  conjunto de nodos sucursales  (comienzan con Su)  en J estan los nombres no la S indicando canal
#  se completan los compartidosCamiones con -1 (sin restriccion) si no se ingreso un dato
#
#  solo se recorta demanda en sucursales  (solo tenemos precios de producto en sucursales)
#        
    JN = {}
    Jsolas = {}
    for j in J:
        if str(j)[:2]=='Su':
            JN[j] = j
            if j not in pareadas:
                Jsolas[j]=j
        if j not in compartidosCamiones:
            compartidosCamiones[j]=-1

    
    citypairN = {}
    for (i,j) in citypair:
        if str(i)[:2]=='Su' and str(j)[:2]=='Su':
            citypairN[i,j]=None


#  Determinar reduccion de demanda   if demanda > 26* numero camiones
            
    MontoRecortePAL = {}
    for j in Jsolas:
        if compartidosCamiones[j]>=0:
            MontoRecortePAL[j] = max(sum(dfx[h,g,jj]/cajaspallet[h] for (h,g,jj) in dfx if jj==j or jj==j+'S')-compartidosCapPal[j]*compartidosCamiones[j],0)
        else:
            MontoRecortePAL[j] = 0
            
    MontoRecorteKG = {}
    for j in Jsolas:
        if compartidosCamiones[j]>=0:
            MontoRecorteKG[j] = max(sum(dfx[h,g,jj]*pesocaja[h] for (h,g,jj) in dfx if jj==j or jj==j+'S')-capKGStandard*compartidosCamiones[j],0)
        else:
            MontoRecorteKG[j] = 0
#
# 
#  Repetir definicion arriba para viajes combinados
#
    MontoRecParesPAL = {}
    for (i,j) in citypairN:
        if compartidosCamiones[i]>= 0 and compartidosCamiones[j] >= 0:
            aux1=sum(dfx[h,g,jj]/cajaspallet[h] for (h,g,jj) in dfx if jj==j or jj==j+'S')
            aux2=sum(dfx[h,g,jj]/cajaspallet[h] for (h,g,jj) in dfx if jj==i or jj==i+'S')
            MontoRecParesPAL[i,j] = max(aux1+aux2-compartidosCapPal[j]*compartidosCamiones[j]-compartidosCapPal[i]*compartidosCamiones[i],0)
        else:
            MontoRecParesPAL[i,j] = 0
            
            
    MontoRecParesKG = {}
    for (i,j) in citypairN:
        if compartidosCamiones[i]>= 0 and compartidosCamiones[j] >= 0:
            aux1=sum(dfx[h,g,jj]*pesocaja[h] for (h,g,jj) in dfx if jj==j or jj==j+'S')
            aux2=sum(dfx[h,g,jj]*pesocaja[h] for (h,g,jj) in dfx if jj==i or jj==i+'S')
            MontoRecParesKG[i,j] = max(aux1+aux2-capKGStandard*(compartidosCamiones[j]+compartidosCamiones[i]),0)
        else:
            MontoRecParesKG[i,j] = 0

#
#    coeficientes para reduccion pareja de demanda si recorte es infactible
#
    coefpal = {}
    coefkg = {}
    for jj in Jsolas:
        auxx = 100*max(pesocaja[h]*(pre[j,h]-c[h]) for (h,g,j) in dfx if j==jj or j==jj+'S')        
        if sum(dfx[h,g,j] for (h,g,j) in dfx if j==jj or j==jj+'S') > 0:
            prop = sum(dfx[h,g,j] for (h,g,j) in dfx if j==jj or j==jj+'S')/sum(dfx[h,g,j]/cajaspallet[h] for (h,g,j) in dfx if j==jj or j==jj+'S')
            coefpal[jj] = auxx*prop            
            prop = sum(dfx[h,g,j] for (h,g,j) in dfx if j==jj or j==jj+'S')/sum(dfx[h,g,j]*pesocaja[h] for (h,g,j) in dfx if j==jj or j==jj+'S')
            coefkg[jj] = auxx*prop            
    coefpalp = {}
    coefkgp = {}
    for (ii,jj) in citypairN:
        auxx = 100*max(pesocaja[h]*(pre[j,h]-c[h]) for (h,g,j) in dfx if j==jj or j==jj+'S' or j==ii or j==ii+'S')
        if sum(dfx[h,g,j] for (h,g,j) in dfx if j==jj or j==jj+'S' or j==ii or j==ii+'S') > 0:
            prop = sum(dfx[h,g,j] for (h,g,j) in dfx if j==jj or j==jj+'S' or j==ii or j==ii+'S')/sum(dfx[h,g,j]/cajaspallet[h] for (h,g,j) in dfx if j==jj or j==jj+'S' or j==ii or j==ii+'S')
            coefpalp[ii,jj] = auxx*prop            
            prop = sum(dfx[h,g,j] for (h,g,j) in dfx if j==jj or j==jj+'S' or j==ii or j==ii+'S')/sum(dfx[h,g,j]*pesocaja[h] for (h,g,j) in dfx if j==jj or j==jj+'S' or j==ii or j==ii+'S')
            coefkgp[ii,jj] = auxx*prop            
#
#####################
            
    if env == None:
        r = Model('ppl')
    else:
        r = Model('ppl', env=env)

    r.setParam('LogFile', pathLog + '/Logprobrecorte.log')

    rec = {}
    for h,g,j in dfx:   # se define para toda sucursal que tenga demanda, con y sin S
        if j in JN:
            rec[h,g,j] = r.addVar(vtype="C",lb=0,name='rec_%s_%s_%s' % (h, g, j))
        elif str(j)[:-1] in JN:
            rec[h,g,j] = r.addVar(vtype="C",lb=0,name='rec_%s_%s_%s' % (h, g, j))
            
    udemrec = {}
    for h,g,j in dfx:   # se define para toda sucursal que tenga demanda, con y sin S
        if j in JN:
            udemrec[h,g,j] = r.addVar(vtype="C",lb=0,name='udemrec_%s_%s_%s' % (h, g, j))    
        elif str(j)[:-1] in JN:
            udemrec[h,g,j] = r.addVar(vtype="C",lb=0,name='udemrec_%s_%s_%s' % (h, g, j))    
            
    infrecPAL = {}
    for j in Jsolas:
        infrecPAL[j] = r.addVar(vtype="C",lb=0,name='infrecPAL_%s' % j)

    infrecKG = {}
    for j in Jsolas:
        infrecKG[j] = r.addVar(vtype="C",lb=0,name='infrecKG_%s' % j)

    infrecPALp = {}
    for (i,j) in citypairN:
        infrecPALp[i,j] = r.addVar(vtype="C",lb=0,name='infrecPALp_%s_%s' % (i,j))

    infrecKGp = {}
    for (i,j) in citypairN:
        infrecKGp[i,j] = r.addVar(vtype="C",lb=0,name='infrecKGp_%s_%s' % (i,j))

        
    for (h,g,j) in rec:
        r.addConstr(rec[h,g,j]+udemrec[h,g,j] <= dfx[h,g,j])

    for (h,g,j) in rec:
        if (j,h) not in maxRecorte:          # no puedo recortar productos que no tienen capacidad de recorte
            maxRecorte[j,h]=0
        
    for (j,h) in maxRecorte:
        if j in JN:
            r.addConstr(quicksum(rec[hh,g,jj] for (hh,g,jj) in rec if hh==h and (jj==j or jj==j+'S')) <= maxRecorte[j,h])

    for j in Jsolas:
        r.addConstr(quicksum(rec[h,g,jj]/cajaspallet[h] for (h,g,jj) in rec if jj==j or jj==j+'S') + quicksum(udemrec[h,g,jj]/cajaspallet[h] for (h,g,jj) in udemrec if jj==j or jj==j+'S') +infrecPAL[j] >= MontoRecortePAL[j])
        r.addConstr(quicksum(rec[h,g,jj]*pesocaja[h] for (h,g,jj) in rec if jj==j or jj==j+'S')    + quicksum(udemrec[h,g,jj]*pesocaja[h] for (h,g,jj) in udemrec if jj==j or jj==j+'S') + infrecKG[j] >= MontoRecorteKG[j])

    for (i,j) in citypairN:
        r.addConstr(quicksum(rec[h,g,jj]/cajaspallet[h] for (h,g,jj) in rec if jj==j or jj==j+'S') + quicksum(udemrec[h,g,jj]/cajaspallet[h] for (h,g,jj) in udemrec if jj==j or jj==j+'S') + quicksum(rec[h,g,jj]/cajaspallet[h] for (h,g,jj) in rec if jj==i or jj==i+'S') + quicksum(udemrec[h,g,jj]/cajaspallet[h] for (h,g,jj) in udemrec if jj==i or jj==i+'S') + infrecPALp[i,j] >= MontoRecParesPAL[i,j])
        r.addConstr(quicksum(rec[h,g,jj]*pesocaja[h] for (h,g,jj) in rec if jj==j or jj==j+'S')    + quicksum(udemrec[h,g,jj]*pesocaja[h] for (h,g,jj) in udemrec if jj==j or  jj==j+'S')   + quicksum(rec[h,g,jj]*pesocaja[h] for (h,g,jj) in rec if jj==i or jj==i+'S')    + quicksum(udemrec[h,g,jj]*pesocaja[h] for (h,g,jj) in udemrec if jj==i or jj==i+'S') + infrecKGp[i,j] >= MontoRecParesKG[i,j])

    for (h,g) in infdemg:
        r.addConstr(quicksum(udemrec[hh,gg,j] for (hh,gg,j) in udemrec if hh==h and gg== g) <= infdemg[h,g])

    #FO DE CV*Y SOLOS
    r.setObjective(quicksum(pesocaja[h]*(pre[j,h]-c[h])*rec[h,g,j] for (h,g,j) in rec)+quicksum(coefpal[j]*infrecPAL[j]+coefkg[j]*infrecKG[j] for j in Jsolas)+quicksum(coefpalp[i,j]*infrecPALp[i,j]+coefkgp[i,j]*infrecKGp[i,j] for (i,j) in citypairN),GRB.MINIMIZE)   

#
#  DEBUG:   check negative coefficients in the objective funcion
#           all are  pre[j,h]-c[h] and should be >= 0 ??
#
    for (h,g,j) in rec:
        if h==1020439:
            print(h,g,j,pre[j,h],c[h])
        if pesocaja[h]*(pre[j,h]-c[h]) < 0:
            print('coef rec[h,g,j] = ', h,g,j,pre[j,h],c[h],pesocaja[h]*(pre[j,h]-c[h] ))

    for j in Jsolas:
        if coefpal[j] < 0:
            print('coef infrecPAL[j] = ', j, coefpal[j])
        if coefkg[j] < 0: 
            print('coef infrecKG[j] = ', j, coefkg[j])
            
    for (i,j) in citypairN:
        if coefpalp[i,j] <0:
            print('coef infrecPALp[i,j] = ', i, j, coefpalp[i,j])
        if coefkgp[i,j] < 0: 
            print('coef infrecKGp[i,j] = ', i, j, coefkgp[i,j])

#
#  end DEBUG ?
#        


    r.optimize()

    status = r.Status
    
    if status != GRB.OPTIMAL:

        ################
        #
        #   recorte no se resolvio a optimalidad.   Dar error y se puede 
        #   tratar de resolver porque.
        #
        
        print('Problema recorte no resuelto a optimalidad')
        print('No se modificaran dfx ni infdemg')

        #if status == GRB.INF_OR_UNBD
        #   r.setParam('DualReductions',0)
        #   r.setParam('InfUnbdInfo',1)
        #
        #   r.optimize()
        ##   si ahora es unbounded se puede recuperar el rayo:
        #
        #   ubdrayrec = r.getAttr('UnbdRay',rec)
        #   ubdrayudemrec = r.getAttr('UnbdRay',udemrec)
        #   ubdrayinfPAL = r.getAttr('UnbdRay',infrecPAL)
        #   ubdrayinfKG = r.getAttr('UnbdRay',infrecKG)
        #   ubdrayinfPALp = r.getAttr('UnbdRay',infrecPALp)
        #   ubdrayinfKGp = r.getAttr('UnbdRay',infrecKGp)
        #
        #   print("Unbounded Ray")
        #   for (h,g,j) in dfx:
        #       if j in JN or str(j)[:-1] in JN:
        #           if ubdrayrec[h,g,j] != 0:
        #               print('Ray_rec(',h,',',g,',',j,') = ', ubdrayrec[h,g,j])
        #   for (h,g,j) in dfx:
        #       if j in JN or str(j)[:-1] in JN:
        #           if ubdrayudemrec[h,g,j] != 0:
        #               print('Ray_udemrec(',h,',',g,',',j,') = ', ubdrayudemrec[h,g,j])
        #            
        #   for j in Jsolas:
        #       if ubdrayinfPAL[j] != 0:
        #           print('Ray_infPAL[',j,'] = ', ubdrayinfPAL[j])
        #       if ubdrayinfKG[j] != 0:
        #           print('Ray_infKG[',j,'] = ', ubdrayinfKG[j])
        #
        #   for (i,j) in citypairN:
        #       if ubdrayinfPALp[i,j] != 0:
        #           print('Ray_infPALp[',i,',',j,'] = ', ubdrayinfPALp[i,j])
        #       if ubdrayinfKGp[i,j] != 0:
        #           print('Ray_infKGp[',i,',',j,'] = ', ubdrayinfKGp[i,j])
        # 
        #   print("cost of unbounded ray, coefpal[''SuT164''] =  ", coefpal['SuT164'])  
        #
        #
        ################
        
    else:
        solrec = r.getAttr('x',rec)
        soludemrec = r.getAttr('x',udemrec)
        solinfPAL = r.getAttr('x',infrecPAL)    
        solinfKG = r.getAttr('x',infrecKG)
        solinfPALp = r.getAttr('x',infrecPALp)    
        solinfKGp = r.getAttr('x',infrecKGp)
    
        print("Resolvio problema recorte, reduce demanda en ", sum(solrec[h,g,j]+soludemrec[h,g,j] for (h,g,j) in solrec), " cajas total")
        print("descontando redondeo, esto es una reduccion de ", sum(solrec[h,g,j] for (h,g,j) in solrec), " cajas de demanda")
        print("y una reduccion de ", sum(soludemrec[h,g,j] for (h,g,j) in soludemrec), " cajas de demanda no satisfecha")
        print("A continuacion infactibilidad de recorte por pallets si existe:")
        for j in Jsolas:
            if solinfPAL[j]>0:
                print("sucursal ", j ," pallets reducidos parejos ", solinfPAL[j])
        for (i,j) in citypairN:
            if solinfPALp[i,j]>0:
                print("sucursales ", i, j ," pallets reducidos parejos ", solinfPAL[i,j])

        logAlertas = open(pathLog + "/problemarecorte.log","w")

        logAlertas.write('Resolvio problema recorte, reduce demanda en ' + str(sum(solrec[h,g,j]+soludemrec[h,g,j] for (h,g,j) in solrec)) + ' cajas total \n')
        logAlertas.write("descontando redondeo, esto es una reduccion de " + str(sum(solrec[h,g,j] for (h,g,j) in solrec)) + " cajas de demanda \n")
        logAlertas.write("y una reduccion de " + str(sum(soludemrec[h,g,j] for (h,g,j) in soludemrec)) + " cajas de demanda no satisfecha\n")
        logAlertas.write("A continuacion infactibilidad de recorte por pallets si existe: \n")
        for j in Jsolas:
            if solinfPAL[j]>0:
                logAlertas.write("sucursal " + str(j) + " pallets reducidos parejos " + str(solinfPAL[j]) + "\n")
        for (i,j) in citypairN:
            if solinfPALp[i,j]>0:
                logAlertas.write("sucursales " + str(i) + " , " + str(j) + " pallets reducidos parejos " + str(solinfPAL[i,j]) + "\n")
        logAlertas.close()

        ####
        #
        #   recortes de demanda
        #
        #    
        for (h,g,j) in solrec:
            dfx[h,g,j] = dfx[h,g,j]-math.ceil(solrec[h,g,j]+soludemrec[h,g,j])
        
        for j in Jsolas:
            if solinfPAL[j]>0:
                alpha1 = solinfPAL[j]/sum(dfx[h,g,jj]/cajaspallet[h] for (h,g,jj) in dfx if jj==j or jj==j+'S')
                alpha2 = solinfKG[j]/sum(dfx[h,g,jj]*pesocaja[h] for (h,g,jj) in dfx if jj==j or jj==j+'S')
                alpha = max(alpha1,alpha2)
                reduct = 0
                if alpha1 < alpha2:
                    limit = solinfKG[j]
                else:
                    limit = solinfPAL[j]
                for (h,g,jj) in dfx:
                    if reduct > limit:
                        break
                    elif (jj==j or jj==j+'S') and dfx[h,g,jj]>0:
                        aux = min(math.ceil(alpha*dfx[h,g,jj]),dfx[h,g,jj])
                        dfx[h,g,jj] = dfx[h,g,jj]-aux
                        if alpha1 < alpha2:
                            reduct = reduct+aux*pesocaja[h]
                        else:
                            reduct = reduct+aux/cajaspallet[h]
                print('redujo demanda en sucursal infactible ', j, ' Se sacaron ', reduct, 'cajas/pallets se tenia que sacar ', limit)

        for (i,j) in citypairN:
            if solinfPALp[i,j]>0:
                alpha1 = solinfPALp[i,j]/sum(dfx[h,g,jj]/cajaspallet[h] for (h,g,jj) in dfx if jj==j or jj==j+'S' or jj==i or jj==i+'S')
                alpha2 = solinfKGp[i,j]/sum(dfx[h,g,jj]*pesocaja[h] for (h,g,jj) in dfx if jj==j or jj==j+'S' or jj==i or jj==i+'S')
                alpha = max(alpha1,alpha2)
                reduct = 0
                if alpha1 < alpha2:
                    limit = solinfKGp[i,j]
                else:
                    limit = solinfPALp[i,j]
                for (h,g,jj) in dfx:
                    if reduct > limit:
                        break
                    elif (jj==j or jj==j+'S' or jj==i or jj==i+'S') and dfx[h,g,jj]>0:
                        aux = min(math.ceil(alpha*dfx[h,g,jj]),dfx[h,g,jj])
                        dfx[h,g,jj] = dfx[h,g,jj]-aux
                        if alpha1 < alpha2:
                            reduct = reduct+aux*pesocaja[h]
                        else:
                            reduct = reduct+aux/cajaspallet[h]
                print('redujo demanda en pares infactibles ', i, j, ' Se sacaron ', reduct, 'cajas/pallets se tenia que sacar ', limit)

        for (h,g) in infdemg:
            infdemg[h,g] = infdemg[h,g]-math.floor(sum(soludemrec[hh,gg,j] for (hh,gg,j) in soludemrec if hh==h and gg==g))


    #
    #  
    
    return dfx, infdemg
# -*- coding: utf-8 -*-
"""
Created on Wed Aug  5 23:43:46 2020

@author: fernando
"""

import numpy as np
from gurobipy import *
from gurobipy import GRB
from gurobipy import Model
from gurobipy import quicksum
from time import time


def cuadratura(JHSET, soly, soly2, solys, solxi, solxri, solx2i, solxsi, ofx, SETD, arcij, arcon, acdir, arcijj, arcsxd, arcsx2d, arcsxrd, arcsxsd, ARCVK, ARCV2K, ARCVSK, I, I0, I1, I2, cons, JuJS, J, K, F, w, congelado, fresco, pesocaja, kg, cajaspallet, cap, mincap, maxcap, kM, MM, pre, c, rec4quad, pathLog, env):
    HSET = {}
    for (j,h) in JHSET:
        HSET[h] = None

    qofx = {}
    for (h,g,i) in ofx:
        if h in HSET:
            oo1 = sum(solxri[h,g,I1[i],I2[l],j] for l in I for j in JuJS if (h,g,I1[i],I2[l],j) in arcsxrd) + sum(solx2i[h,g,I1[i],I2[l],j] for l in I for j in JuJS if (h,g,I1[i],I2[l],j) in arcsx2d) 
            oo2 = sum(solxi[h,g,I2[i],j] for j in JuJS if (h,g,I2[i],j) in arcsxd) + sum(solxsi[h,g,jd,I2[i],j1,j2] for j1 in JuJS for j2 in JuJS for jd in {j1,j2,str(j1)+'S',str(j2)+'S'} if (h,g,jd,I2[i],j1,j2) in arcsxsd)   
            qofx[h,g,i] = ofx[h,g,i] - oo1 - oo2
 
    solp = {}
    solz = {}
    for i,j in arcij:
        for f in list(w.keys()):
            solp[i,j,f] = sum(solxi[h,ds,i,j]*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,i,j) in arcsxd) + sum(solxi[h,ds,i,str(j)+'S']*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,i,str(j)+'S') in arcsxd)
            solz[i,j,f] = sum(solxi[h,ds,i,j]/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,i,j) in arcsxd) + sum(solxi[h,ds,i,str(j)+'S']/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,i,str(j)+'S') in arcsxd)


    print('sigue cuadratura ')

    solp2 = {}
    solz2 = {}
    solpr = {}
    solzr = {}            
    for l,i,j in acdir:
        for f in list(w.keys()):
            solp2[l,i,j,f] = sum(solx2i[h,ds,l,i,j]*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,l,i,j) in arcsx2d) + sum(solx2i[h,ds,l,i,str(j)+'S']*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,l,i,str(j)+'S') in arcsx2d)
            solz2[l,i,j,f] = sum(solx2i[h,ds,l,i,j]/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,l,i,j) in arcsx2d) + sum(solx2i[h,ds,l,i,str(j)+'S']/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,l,i,str(j)+'S') in arcsx2d)  
            if i in cons:
                solpr[l,i,j,f] = sum(solxri[h,ds,l,i,j]*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,l,i,j) in arcsxrd) + sum(solxri[h,ds,l,i,str(j)+'S']*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,l,i,str(j)+'S') in arcsxrd)
                solzr[l,i,j,f] = sum(solxri[h,ds,l,i,j]/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,l,i,j) in arcsxrd) + sum(solxri[h,ds,l,i,str(j)+'S']/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,l,i,str(j)+'S') in arcsxrd)  

    solps = {}
    solzs = {}            
    for i,j1,j2 in arcijj:
        for f in list(w.keys()):
            solps[i,j1,j2,f] = sum(solxsi[h,g,jd,i,j1,j2]*pesocaja[h] for h in w[f] for g in SETD for jd in {j1,j2,str(j1)+'S',str(j2)+'S'} if (h,g,jd,i,j1,j2) in arcsxsd)
            solzs[j1,i,j1,j2,f] = sum(solxsi[h,g,jd,i,j1,j2]/cajaspallet[h] for h in w[f] for g in SETD for jd in {j1,str(j1)+'S'} if (h,g,jd,i,j1,j2) in arcsxsd) 
            solzs[j2,i,j1,j2,f] = sum(solxsi[h,g,jd,i,j1,j2]/cajaspallet[h] for h in w[f] for g in SETD for jd in {j2,str(j2)+'S'} if (h,g,jd,i,j1,j2) in arcsxsd) 


    if env == None:
        qm = Model('ppl')
    else:
        qm = Model('ppl', env=env)
    qm.setParam('LogFile', pathLog + '/Lognobajaquad.log')

    qarcsxd = {}
    for (h,g,i,j) in arcsxd:          
        if (j,h) in JHSET:         # restringir mas aun:  i in cons or h,i in qofx and qofx > 0
            qarcsxd[h,g,i,j] = None
            
    qx = {}
    for h,g,i,j in qarcsxd:
            qx[h,g,i,j] = qm.addVar(vtype="C",lb=0,name='qx[%s,%s,%s,%s]' % (h, g, i, j))

    qarcsxrd = {}
    for (h,g,l,i,j) in arcsxrd:
        if (j,h) in JHSET and (h,g,I0[l]) in qofx and qofx[h,g,I0[l]]>0: 
            qarcsxrd[h,g,l,i,j] = None
            
    qxr = {}
    for h,g,l,i,j in qarcsxrd:
            qxr[h,g,l,i,j] = qm.addVar(vtype="C",lb=0,name='qxr[%s,%s,%s,%s,%s]' % (h, g, l, i, j))

    qarcsx2d = {}
    for (h,g,l,i,j) in arcsx2d:
        if (j,h) in JHSET and (h,g,I0[l]) in qofx and qofx[h,g,I0[l]]>0: 
            qarcsx2d[h,g,l,i,j] = None
            
    qx2 = {}
    for h,g,l,i,j in qarcsx2d:
            qx2[h,g,l,i,j] = qm.addVar(vtype="C",lb=0,name='qx2[%s,%s,%s,%s,%s]' % (h, g, l, i, j))

    qarcsxsd = {}
    for (h,g,jd,i,j1,j2) in arcsxsd:
        if (jd,h) in JHSET and (h,g,I0[i]) in qofx and qofx[h,g,I0[i]]>0: 
            qarcsxsd[h,g,jd,i,j1,j2] = None
            
    qxs = {}
    for h,g,jd,i,j1,j2 in qarcsxsd:
            qxs[h,g,jd,i,j1,j2] = qm.addVar(vtype="C",lb=0,name='qxs[%s,%s,%s,%s,%s,%s]' % (h, g, jd, i, j1, j2))
             
    qp = {} 
    qz = {} 
    qzc = {} 
    for i,j in arcij:
        qzc[i,j] = qm.addVar(vtype="I",lb=0,name='qzc[%s,%s]' % (i, j))   
        for f in F:
            qp[i,j,f] = qm.addVar(vtype="C",lb=0,name='qp[%s,%s,%s]' % (i, j, f))   
            qz[i,j,f] = qm.addVar(vtype="I",lb=0,name='qz[%s,%s,%s]' % (i, j, f))   

    qp2 = {} 
    qz2 = {} 
    qz2c = {} 
    qpr = {} 
    qzr = {} 
    qzrc = {} 
    for l,i,j in acdir:
        qz2c[l,i,j] = qm.addVar(vtype="I",lb=0,name='qz2c[%s,%s,%s]' % (l, i, j))   
        for f in F:
            qp2[l,i,j,f] = qm.addVar(vtype="C",lb=0,name='qp2[%s,%s,%s,%s]' % (l, i, j, f))   
            qz2[l,i,j,f] = qm.addVar(vtype="I",lb=0,name='qz2[%s,%s,%s,%s]' % (l, i, j, f))
        if i in cons:
            qzrc[l,i,j] = qm.addVar(vtype="I",lb=0,name='qzrc[%s,%s,%s]' % (l, i, j))   
            for f in F:
                qpr[l,i,j,f] = qm.addVar(vtype="C",lb=0,name='qpr[%s,%s,%s,%s]' % (l, i, j, f))   
                qzr[l,i,j,f] = qm.addVar(vtype="I",lb=0,name='qzr[%s,%s,%s,%s]' % (l, i, j, f))

    qps = {} 
    qzs = {} 
    qzsc = {} 
    for i,j1,j2 in arcijj:
        qzsc[i,j1,j2] = qm.addVar(vtype="I",lb=0,name='qzsc[%s,%s,%s]' % (i, j1, j2))   
        for f in F:
            qps[i,j1,j2,f] = qm.addVar(vtype="C",lb=0,name='qps[%s,%s,%s,%s]' % (i, j1, j2, f))
            for jd in {j1,j2}:
                qzs[jd,i,j1,j2,f] = qm.addVar(vtype="I",lb=0,name='qzs[%s,%s,%s,%s,%s]' % (jd, i, j1, j2, f))


    qzMc = {}
    qzNc = {}
    qzIc = {}
    qzFc = {}
    for i,j in arcij:
        qzMc[i,j] = qm.addVar(vtype="I",lb=0,name='qzMc[%s,%s]' % (i, j))   
        qzNc[i,j] = qm.addVar(vtype="I",lb=0,name='qzNc[%s,%s]' % (i, j))   
        qzIc[i,j] = qm.addVar(vtype="B",lb=0,name='qzIc[%s,%s]' % (i, j))   
        qzFc[i,j] = qm.addVar(vtype="B",lb=0,name='qzFc[%s,%s]' % (i, j))   

    qzMrc = {}
    qzNrc = {}
    qzIrc = {}
    qzFrc = {}
    for l,i in arcon:
        qzMrc[l,i] = qm.addVar(vtype="I",lb=0,name='qzMrc[%s,%s]' % (l, i))
        qzNrc[l,i] = qm.addVar(vtype="I",lb=0,name='qzNrc[%s,%s]' % (l, i))
        qzIrc[l,i] = qm.addVar(vtype="B",lb=0,name='qzIrc[%s,%s]' % (l, i))
        qzFrc[l,i] = qm.addVar(vtype="B",lb=0,name='qzFrc[%s,%s]' % (l, i))

    qzM2c = {} 
    qzN2c = {} 
    qzI2c = {} 
    qzF2c = {}
    for l,i,j in acdir:
        qzM2c[l,i,j] = qm.addVar(vtype="I",lb=0,name='qzM2c[%s,%s,%s]' % (l, i, j))   
        qzN2c[l,i,j] = qm.addVar(vtype="I",lb=0,name='qzN2c[%s,%s,%s]' % (l, i, j))   
        qzI2c[l,i,j] = qm.addVar(vtype="B",lb=0,name='qzI2c[%s,%s,%s]' % (l, i, j))   
        qzF2c[l,i,j] = qm.addVar(vtype="B",lb=0,name='qzF2c[%s,%s,%s]' % (l, i, j))   

    qzMsc = {} 
    qzNsc = {} 
    qzIsc = {} 
    qzFsc = {}
    for i,j1,j2 in arcijj:
        qzMsc[i,j1,j2] = qm.addVar(vtype="I",lb=0,name='qzMsc[%s,%s,%s]' % (i, j1, j2))   
        qzNsc[i,j1,j2] = qm.addVar(vtype="I",lb=0,name='qzNsc[%s,%s,%s]' % (i, j1, j2))   
        qzIsc[i,j1,j2] = qm.addVar(vtype="B",lb=0,name='qzIsc[%s,%s,%s]' % (i, j1, j2))   
        qzFsc[i,j1,j2] = qm.addVar(vtype="B",lb=0,name='qzFsc[%s,%s,%s]' % (i, j1, j2))   


            
    qo1 = {}
    qo2 = {}
    for (h,g,i) in qofx:
        qo1[h,g,i] = qm.addVar(vtype="C",lb=0,name='qo1[%s,%s,%s]' % (h,g,i))
        qo2[h,g,i] = qm.addVar(vtype="C",lb=0,name='qo2[%s,%s,%s]' % (h,g,i))
        

    print('Se definieron las variables en cuadratura')    
    #RESTRICCIONES

    #Oferta 
    for (h,g,i) in qofx:
        qm.addConstr(qo1[h,g,i]+qo2[h,g,i] <= qofx[h,g,i])

        qm.addConstr( quicksum(qxr[h,g,I1[i],I2[l],j] for l in I for j in JuJS if (h,g,I1[i],I2[l],j) in qarcsxrd) + quicksum(qx2[h,g,I1[i],I2[l],j] for l in I for j in JuJS if (h,g,I1[i],I2[l],j) in qarcsx2d) <= qo1[h,g,i] )
        qm.addConstr( quicksum(qx[h,g,I2[i],j] for j in JuJS if (h,g,I2[i],j) in qarcsxd) + quicksum(qxs[h,g,jd,I2[i],j1,j2] for j1 in JuJS for j2 in JuJS for jd in {j1,j2,str(j1)+'S',str(j2)+'S'} if (h,g,jd,I2[i],j1,j2) in qarcsxsd) <= qo2[h,g,i] )
       

    # En cuadaratura no hay demanda, se envia todo lo rentable de una lista de productos.
    ###########


    for i,j in arcij:
        qm.addConstr(quicksum(qz[i,j,f] for f in congelado) + quicksum(qzr[I1[l],i,j,f] for f in congelado for l in I1 if (I1[l],i,j,f) in qzr)  <= 2*qzc[i,j] )
        for f in list(w.keys()):
            qm.addConstr(solp[i,j,f]+quicksum(qx[h,ds,i,j]*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,i,j) in qarcsxd) + quicksum(qx[h,ds,i,str(j)+'S']*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,i,str(j)+'S') in qarcsxd) <= qp[i,j,f])
            qm.addConstr(solz[i,j,f]+quicksum(qx[h,ds,i,j]/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,i,j) in qarcsxd) + quicksum((qx[h,ds,i,str(j)+'S']/cajaspallet[h]) for h in w[f] for ds in SETD if (h,ds,i,str(j)+'S') in qarcsxd) <= qz[i,j,f] )

    for l,i,j in acdir:        
        qm.addConstr(quicksum(qz2[l,i,j,f] for f in congelado) <= 2*qz2c[l,i,j] )
        if i in cons:
            qm.addConstr(quicksum(qzr[l,i,j,f] for f in congelado) <= 2*qzrc[l,i,j] )
        for f in list(w.keys()):
            qm.addConstr(solp2[l,i,j,f] + quicksum(qx2[h,ds,l,i,j]*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,l,i,j) in qarcsx2d) + quicksum(qx2[h,ds,l,i,str(j)+'S']*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,l,i,str(j)+'S') in qarcsx2d) <= qp2[l,i,j,f])
            qm.addConstr(solz2[l,i,j,f] + quicksum(qx2[h,ds,l,i,j]/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,l,i,j) in qarcsx2d) + quicksum(qx2[h,ds,l,i,str(j)+'S']/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,l,i,str(j)+'S') in qarcsx2d) <= qz2[l,i,j,f] )
            if i in cons:
                qm.addConstr(solpr[l,i,j,f] + quicksum(qxr[h,ds,l,i,j]*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,l,i,j) in qarcsxrd) + quicksum(qxr[h,ds,l,i,str(j)+'S']*pesocaja[h] for h in w[f] for ds in SETD if (h,ds,l,i,str(j)+'S') in qarcsxrd) <= qpr[l,i,j,f])
                qm.addConstr(solzr[l,i,j,f] + quicksum(qxr[h,ds,l,i,j]/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,l,i,j) in qarcsxrd) + quicksum(qxr[h,ds,l,i,str(j)+'S']/cajaspallet[h] for h in w[f] for ds in SETD if (h,ds,l,i,str(j)+'S') in qarcsxrd) <= qzr[l,i,j,f] )

    for i,j1,j2 in arcijj:
        qm.addConstr(quicksum(qzs[jd,i,j1,j2,f] for jd in {j1,j2} for f in congelado) <= 2*qzsc[i,j1,j2] )
        for f in list(w.keys()):
            qm.addConstr(solps[i,j1,j2,f] + quicksum(qxs[h,g,jd,i,j1,j2]*pesocaja[h] for h in w[f] for g in SETD for jd in {j1, j2, str(j1)+'S', str(j2)+'S'} if (h,g,jd,i,j1,j2) in qarcsxsd) <= qps[i,j1,j2,f])
            qm.addConstr(solzs[j1,i,j1,j2,f] + quicksum((qxs[h,g,jd,i,j1,j2]/cajaspallet[h]) for h in w[f] for g in SETD for jd in {j1, str(j1)+'S'} if (h,g,jd,i,j1,j2) in qarcsxsd) <= qzs[j1,i,j1,j2,f] )
            qm.addConstr(solzs[j2,i,j1,j2,f] + quicksum((qxs[h,g,jd,i,j1,j2]/cajaspallet[h]) for h in w[f] for g in SETD for jd in {j2, str(j2)+'S'} if (h,g,jd,i,j1,j2) in qarcsxsd) <= qzs[j2,i,j1,j2,f] )


        
    for i,j in arcij:
        if i in cons:
            qm.addConstr(quicksum(qp[i,j,f] for f in F) + quicksum(qp2[I1[l],i,j,f] for f in F for l in I1 if (I1[l],i,j) in acdir) + quicksum(qpr[I1[l],i,j,f] for f in F for l in I1 if (I1[l],i,j) in acdir) <= sum(kg[k]*soly[i,j,k] for k in K if (i,j,k) in ARCVK) + sum(kg[k]*soly2[I1[l],i,j,k] for k in K for l in I1 if (I1[l],i,j,k) in ARCV2K))
            qm.addConstr(quicksum(qz[i,j,f] for f in fresco) + quicksum(qz2[I1[l],i,j,f] for f in fresco for l in I1 if (I1[l],i,j) in acdir) + quicksum(qzr[I1[l],i,j,f] for f in fresco for l in I1 if (I1[l],i,j) in acdir) + 2*(qzMc[i,j] + qzNc[i,j]) + quicksum(2*(qzM2c[I1[l],i,j] + qzN2c[I1[l],i,j]) for l in I1 if (I1[l],i,j) in acdir) <= sum(cap[k]*soly[i,j,k] for k in K if (i,j,k) in ARCVK) + sum(cap[k]*soly2[I1[l],i,j,k] for k in K for l in I1 if (I1[l],i,j,k) in ARCV2K))
        else:
            qm.addConstr(quicksum(qp[i,j,f] for f in F) + quicksum(qp2[I1[l],i,j,f] for f in F for l in I1 if (I1[l],i,j) in acdir) <= sum(kg[k]*soly[i,j,k] for k in K if (i,j,k) in ARCVK) + sum(kg[k]*soly2[I1[l],i,j,k] for k in K for l in I1 if (I1[l],i,j,k) in ARCV2K))
            qm.addConstr(quicksum(qz[i,j,f] for f in fresco) + quicksum(qz2[I1[l],i,j,f] for f in fresco for l in I1 if (I1[l],i,j) in acdir) + 2*(qzMc[i,j] + qzNc[i,j]) + quicksum(2*(qzM2c[I1[l],i,j] + qzN2c[I1[l],i,j]) for l in I1 if (I1[l],i,j) in acdir) <= sum(cap[k]*soly[i,j,k] for k in K if (i,j,k) in ARCVK) + sum(cap[k]*soly2[I1[l],i,j,k] for k in K for l in I1 if (I1[l],i,j,k) in ARCV2K))
        qm.addConstr(2*qzc[i,j] <= sum(cap[k]*soly[i,j,k] for k in K if (i,j,k) in ARCVK))

    for l,i in arcon:
        qm.addConstr(quicksum(qzr[l,i,j,f] for f in fresco for j in JuJS if (l,i,j) in acdir) + 2*(qzMrc[l,i]+qzNrc[l,i]) <= sum(cap[k]*soly[l,i,k] for k in K if (l,i,k) in ARCVK))     
        qm.addConstr(quicksum(qpr[l,i,j,f] for f in F for j in JuJS if (l,i,j) in acdir)<=sum(kg[k]*soly[l,i,k] for k in K if (l,i,k) in ARCVK)) 

    for l,i,j in acdir:
        qm.addConstr(quicksum(qz2[l,i,j,f] for f in fresco)+2*(qzM2c[l,i,j]+qzN2c[l,i,j]) <= sum(cap[k]*soly2[l,i,j,k] for k in K if (l,i,j,k) in ARCV2K))     
        qm.addConstr(quicksum(qp2[l,i,j,f] for f in F)<=sum(kg[k]*soly2[l,i,j,k] for k in K if (l,i,j,k) in ARCV2K)) 

    for i,j1,j2 in arcijj:
        qm.addConstr(quicksum(qps[i,j1,j2,f] for f in F) <= sum(kg[k]*solys[i,j1,j2,k] for k in K if (i,j1,j2,k) in ARCVSK)) 
        qm.addConstr(quicksum(qzs[jd,i,j1,j2,f] for jd in {j1,j2} for f in fresco) + 2*(qzMsc[i,j1,j2]+qzNsc[i,j1,j2]) <= sum(cap[k]*solys[i,j1,j2,k] for k in K if (i,j1,j2,k) in ARCVSK)) 


#    kM = '26P'
    CAPCAM = cap[kM]
    LIMINFCAP = mincap[kM]
    DELTACAP = CAPCAM - maxcap[kM]
#    MM = 1+2*max(math.ceil(sum(dfx[h,g,j]/cajaspallet[h] for (h,g,j) in dfx if j==jj or j==str(jj)+'S')/CAPCAM) for jj in J)

    for l,i in arcon:
        qm.addConstr(quicksum(qzrc[l,i,j] for j in JuJS if (l,i,j) in acdir) <= qzMrc[l,i]+qzNrc[l,i])
        qm.addConstr(qzMrc[l,i] <= CAPCAM*MM*qzIrc[l,i])
        qm.addConstr(qzNrc[l,i] <= CAPCAM*MM*(1-qzIrc[l,i]) )
        if (l,i,kM) in ARCVK:
            qm.addConstr(2*qzMrc[l,i] >= LIMINFCAP*qzIrc[l,i])
            qm.addConstr(2*qzMrc[l,i] >= CAPCAM*soly[l,i,kM] - CAPCAM*MM*(1-qzFrc[l,i]))
            qm.addConstr(2*qzMrc[l,i] <= CAPCAM*soly[l,i,kM] - DELTACAP*(1-qzFrc[l,i]))
        qm.addConstr(2*qzNrc[l,i] <= quicksum(cap[k]*soly[l,i,k] for k in K if (l,i,k) in ARCVK and k!= kM))

    for l,i,j in acdir:
        qm.addConstr(qz2c[l,i,j] <= qzM2c[l,i,j]+qzN2c[l,i,j])
        qm.addConstr(qzM2c[l,i,j] <= CAPCAM*MM*qzI2c[l,i,j])
        qm.addConstr(qzN2c[l,i,j] <= CAPCAM*MM*(1-qzI2c[l,i,j]))
        if (l,i,j,kM) in ARCV2K:
            qm.addConstr(2*qzM2c[l,i,j] >= LIMINFCAP*qzI2c[l,i,j])
            qm.addConstr(2*qzM2c[l,i,j] >= CAPCAM*soly2[l,i,j,kM] - CAPCAM*MM*(1-qzF2c[l,i,j]))
            qm.addConstr(2*qzM2c[l,i,j] <= CAPCAM*soly2[l,i,j,kM] - DELTACAP*(1-qzF2c[l,i,j]))
        qm.addConstr(2*qzN2c[l,i,j] <= quicksum(cap[k]*soly2[l,i,j,k] for k in K if (l,i,j,k) in ARCV2K and k!= kM))

    for i,j in arcij:
        qm.addConstr(qzc[i,j] <= qzMc[i,j]+qzNc[i,j]) 
        qm.addConstr(qzMc[i,j] <= CAPCAM*MM*qzIc[i,j])
        qm.addConstr(qzNc[i,j] <= CAPCAM*MM*(1-qzIc[i,j]))
        if (i,j,kM) in ARCVK:
            qm.addConstr(2*qzMc[i,j] >=  LIMINFCAP*qzIc[i,j])
            qm.addConstr(2*qzMc[i,j] >= CAPCAM*(soly[i,j,kM]) - CAPCAM*MM*(1-qzFc[i,j]))
            qm.addConstr(2*qzMc[i,j] <= CAPCAM*(soly[i,j,kM])  - DELTACAP*(1-qzFc[i,j]))
        qm.addConstr(2*qzNc[i,j] <= quicksum(cap[k]*soly[i,j,k] for k in K if (i,j,k) in ARCVK and k!=kM)  )

    for i,j1,j2 in arcijj:
        qm.addConstr(qzsc[i,j1,j2] <= qzMsc[i,j1,j2]+qzNsc[i,j1,j2])
        qm.addConstr(qzMsc[i,j1,j2] <= CAPCAM*MM*qzIsc[i,j1,j2])
        qm.addConstr(qzNsc[i,j1,j2] <= CAPCAM*MM*(1-qzIsc[i,j1,j2]))
        if (i,j1,j2,kM) in ARCVSK:
            qm.addConstr(2*qzMsc[i,j1,j2] >= LIMINFCAP*qzIsc[i,j1,j2])
            qm.addConstr(2*qzMsc[i,j1,j2] >= CAPCAM*solys[i,j1,j2,kM] - CAPCAM*MM*(1-qzFsc[i,j1,j2]))
            qm.addConstr(2*qzMsc[i,j1,j2] <= CAPCAM*solys[i,j1,j2,kM] - DELTACAP*(1-qzFsc[i,j1,j2]))
        qm.addConstr(2*qzNsc[i,j1,j2] <= quicksum(cap[k]*solys[i,j1,j2,k] for k in K if (i,j1,j2,k) in ARCVSK and k!= kM))



    #
    #
    for j,h in JHSET:
        qm.addConstr(quicksum(qx[h,g,I2[i],j] for g in SETD for i in I2 if (h,g,I2[i],j) in qarcsxd) + quicksum(qx2[h,g,I1[l],I2[i],j] for g in SETD for l in I1 for i in I2 if (h,g,I1[l],I2[i],j) in qarcsx2d) + quicksum(qxr[h,g,I1[l],I2[i],j] for g in SETD for l in I1 for i in I2 if (h,g,I1[l],I2[i],j) in qarcsxrd) + quicksum(qxs[hh,g,jj,i,j1,j2] for (hh,g,jj,i,j1,j2) in qarcsxsd if jj==j and hh==h) <= JHSET[j,h]+rec4quad[j,h]) 

    #
    #FO rentabilidad de productos enviados: maximizar precio de venta - costo produccion
    qm.setObjective(quicksum(pesocaja[h]*(pre[j,h]-c[h])*qx[h,g,i,j] for (h,g,i,j) in qarcsxd if (j,h) in JHSET) + quicksum(pesocaja[h]*(pre[j,h]-c[h])*qx2[h,g,l,i,j] for (h,g,l,i,j) in qarcsx2d if (j,h) in JHSET) + quicksum(pesocaja[h]*(pre[j,h]-c[h])*qxr[h,g,l,i,j] for (h,g,l,i,j) in qarcsxrd if (j,h) in JHSET) + quicksum(pesocaja[h]*(pre[jd,h]-c[h])*qxs[h,g,jd,i,j1,j2] for (h,g,jd,i,j1,j2) in qarcsxsd if (jd,h) in JHSET) - quicksum(qz[i,j,f]*0.1 for (i,j) in arcij for f in F)-quicksum(qzMc[i,j]*0.1+qzNc[i,j]*0.2 for (i,j) in arcij) -quicksum(qz2[l,i,j,f]*0.1 for (l,i,j) in acdir for f in F)-quicksum(qzM2c[l,i,j]*0.1+qzN2c[l,i,j]*0.2 for (l,i,j) in acdir) -quicksum(qzr[l,i,j,f]*0.1 for (l,i,j) in acdir for f in F if i in cons)-quicksum(qzMrc[l,i]*0.1+qzNrc[l,i]*0.2 for (l,i) in arcon if i in cons)-quicksum(qzs[jd,i,j1,j2,f]*0.1 for (i,j1,j2) in arcijj for jd in {j1,j2} for f in F)-quicksum(qzMsc[i,j1,j2]*0.1+qzNsc[i,j1,j2]*0.2 for (i,j1,j2) in arcijj),GRB.MAXIMIZE)   




    qm.setParam('TimeLimit', 300)    # 5 minutos

    qm.setParam('MIPFocus', 2)
    qm.setParam('Cuts', 3)
    qm.setParam('GomoryPasses',1)
    qm.setParam('Heuristics',0.5)
    qm.setParam('NormAdjust',2)
    qm.setParam('PreDual',1)
    qm.setParam('MIPGap',0.000001)

    qm.optimize()

    return qm


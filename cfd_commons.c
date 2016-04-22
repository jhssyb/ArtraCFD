/****************************************************************************
 *                              ArtraCFD                                    *
 *                          <By Huangrui Mo>                                *
 * Copyright (C) Huangrui Mo <huangrui.mo@gmail.com>                        *
 * This file is part of ArtraCFD.                                           *
 * ArtraCFD is free software: you can redistribute it and/or modify it      *
 * under the terms of the GNU General Public License as published by        *
 * the Free Software Foundation, either version 3 of the License, or        *
 * (at your option) any later version.                                      *
 ****************************************************************************/
/****************************************************************************
 * Required Header Files
 ****************************************************************************/
#include "cfd_commons.h"
#include <stdio.h> /* standard library for input and output */
#include <math.h> /* common mathematical functions */
#include "commons.h"
/****************************************************************************
 * Function Pointers
 ****************************************************************************/
/*
 * Function pointers are useful for implementing a form of polymorphism.
 * They are mainly used to reduce or avoid switch statement. Pointers to
 * functions can get rather messy. Declaring a typedef to a function pointer
 * generally clarifies the code.
 */
typedef void (*EigenvalueSplitter)(const Real[restrict], Real [restrict], Real [restrict]);
typedef void (*EigenvectorLComputer)(const Real, const Real, const Real, const Real, 
        const Real, const Real, const Real, Real [restrict][DIMU]);
typedef void (*EigenvectorRComputer)(const Real, const Real, const Real, const Real,
        const Real, const Real, Real [restrict][DIMU]);
typedef void (*ConvectiveFluxComputer)(const Real, const Real, const Real, const Real, 
        const Real, const Real, Real [restrict]);
typedef void (*DiffusiveFluxComputer)(const int, const int, const int, 
        const Space *, const Model *, Real [restrict]);
/****************************************************************************
 * Static Function Declarations
 ****************************************************************************/
static void LocalLaxFriedrichs(const Real [restrict], Real [restrict], Real [restrict]);
static void StegerWarming(const Real [restrict], Real [restrict], Real [restrict]);
static void EigenvectorLZ(const Real, const Real, const Real, const Real, 
        const Real, const Real, const Real, Real [restrict][DIMU]);
static void EigenvectorLY(const Real, const Real, const Real, const Real, 
        const Real, const Real, const Real, Real [restrict][DIMU]);
static void EigenvectorLX(const Real, const Real, const Real, const Real, 
        const Real, const Real, const Real, Real [restrict][DIMU]);
static void EigenvectorRZ(const Real, const Real, const Real, const Real,
        const Real, const Real, Real [restrict][DIMU]);
static void EigenvectorRY(const Real, const Real, const Real, const Real,
        const Real, const Real, Real [restrict][DIMU]);
static void EigenvectorRX(const Real, const Real, const Real, const Real,
        const Real, const Real, Real [restrict][DIMU]);
static void ConvectiveFluxZ(const Real, const Real, const Real, const Real, 
        const Real, const Real, Real [restrict]);
static void ConvectiveFluxY(const Real, const Real, const Real, const Real, 
        const Real, const Real, Real [restrict]);
static void ConvectiveFluxX(const Real, const Real, const Real, const Real, 
        const Real, const Real, Real [restrict]);
static Real Viscosity(const Real);
static Real PrandtlNumber(void);

/****************************************************************************
 * Global Variables Definition with Private Scope
 ****************************************************************************/
static EigenvalueSplitter SplitEigenvalue[2] = {
    LocalLaxFriedrichs,
    StegerWarming};
static EigenvectorLComputer ComputeEigenvectorL[DIMS] = {
    EigenvectorLX,
    EigenvectorLY,
    EigenvectorLZ};
static EigenvectorRComputer ComputeEigenvectorR[DIMS] = {
    EigenvectorRX,
    EigenvectorRY,
    EigenvectorRZ};
static ConvectiveFluxComputer ComputeConvectiveFlux[DIMS] = {
    ConvectiveFluxX,
    ConvectiveFluxY,
    ConvectiveFluxZ};
/****************************************************************************
 * Function definitions
 ****************************************************************************/
void SymmetricAverage(const int averager, const Real gamma, 
        const Real UL[restrict], const Real UR[restrict], Real Uo[restrict])
{
    const Real rhoL = UL[0];
    const Real uL = UL[1] / rhoL;
    const Real vL = UL[2] / rhoL;
    const Real wL = UL[3] / rhoL;
    const Real hTL = (UL[4] / rhoL) * gamma - 0.5 * (uL * uL + vL * vL + wL * wL) * (gamma - 1.0);
    const Real rhoR = UR[0];
    const Real uR = UR[1] / rhoR;
    const Real vR = UR[2] / rhoR;
    const Real wR = UR[3] / rhoR;
    const Real hTR = (UR[4] / rhoR) * gamma - 0.5 * (uR * uR + vR * vR + wR * wR) * (gamma - 1.0);
    Real D = 1.0; /* default is arithmetic mean */
    if (1 == averager) { /* Roe average */
        D = sqrt(rhoR / rhoL);
    }
    Uo[1] = (uL + D * uR) / (1.0 + D); /* u average */
    Uo[2] = (vL + D * vR) / (1.0 + D); /* v average */
    Uo[3] = (wL + D * wR) / (1.0 + D); /* w average */
    Uo[4] = (hTL + D * hTR) / (1.0 + D); /* hT average */
    Uo[5] = sqrt((gamma - 1.0) * (Uo[4] - 0.5 * (Uo[1] * Uo[1] + Uo[2] * Uo[2] + Uo[3] * Uo[3]))); /* the speed of sound */
    return;
}
void Eigenvalue(const int s, const Real Uo[restrict], Real Lambda[restrict])
{
    Lambda[0] = Uo[s+1] - Uo[5];
    Lambda[1] = Uo[s+1];
    Lambda[2] = Uo[s+1];
    Lambda[3] = Uo[s+1];
    Lambda[4] = Uo[s+1] + Uo[5];
    return;
}
void EigenvalueSplitting(const int splitter, const Real Lambda[restrict],
        Real LambdaP[restrict], Real LambdaN[restrict])
{
    SplitEigenvalue[splitter](Lambda, LambdaP, LambdaN);
    return;
}
static void LocalLaxFriedrichs(const Real Lambda[restrict],
        Real LambdaP[restrict], Real LambdaN[restrict])
{
    /* set local maximum as (|Vs| + c) */
    const Real lambdaStar = fabs(Lambda[2]) + Lambda[4] - Lambda[2];
    for (int row = 0; row < DIMU; ++row) {
        LambdaP[row] = 0.5 * (Lambda[row] + lambdaStar);
        LambdaN[row] = 0.5 * (Lambda[row] - lambdaStar);
    }
    return;
}
static void StegerWarming(const Real Lambda[restrict],
        Real LambdaP[restrict], Real LambdaN[restrict])
{
    const Real epsilon = 1.0e-3;
    for (int row = 0; row < DIMU; ++row) {
        LambdaP[row] = 0.5 * (Lambda[row] + sqrt(Lambda[row] * Lambda[row] + epsilon * epsilon));
        LambdaN[row] = 0.5 * (Lambda[row] - sqrt(Lambda[row] * Lambda[row] + epsilon * epsilon));
    }
    return;
}
void EigenvectorL(const int s, const Real gamma, const Real Uo[restrict], Real L[restrict][DIMU])
{
    const Real u = Uo[1];
    const Real v = Uo[2];
    const Real w = Uo[3];
    const Real c = Uo[5];
    const Real q = 0.5 * (u * u + v * v + w * w);
    const Real b = (gamma - 1.0) / (2.0 * c * c);
    const Real d = (1.0 / (2.0 * c)); 
    ComputeEigenvectorL[s](u, v, w, c, q, b, d, L);
    return;
}
static void EigenvectorLZ(const Real u, const Real v, const Real w, const Real c, 
        const Real q, const Real b, const Real d, Real L[restrict][DIMU])
{
    L[0][0] = b * q + d * w;   L[0][1] = -b * u;             L[0][2] = -b * v;             L[0][3] = -b * w - d;     L[0][4] = b;
    L[1][0] = -2 * b * q * u;  L[1][1] = 2 * b * u * u + 1;  L[1][2] = 2 * b * v * u;      L[1][3] = 2 * b * w * u;  L[1][4] = -2 * b * u;
    L[2][0] = -2 * b * q * v;  L[2][1] = 2 * b * v * u;      L[2][2] = 2 * b * v * v + 1;  L[2][3] = 2 * b * w * v;  L[2][4] = -2 * b * v;
    L[3][0] = -2 * b * q + 1;  L[3][1] = 2 * b * u;          L[3][2] = 2 * b * v;          L[3][3] = 2 * b * w;      L[3][4] = -2 * b;
    L[4][0] = b * q - d * w;   L[4][1] = -b * u;             L[4][2] = -b * v;             L[4][3] = -b * w + d;     L[4][4] = b;
    return;
}
static void EigenvectorLY(const Real u, const Real v, const Real w, const Real c, 
        const Real q, const Real b, const Real d, Real L[restrict][DIMU])
{
    L[0][0] = b * q + d * v;    L[0][1] = -b * u;             L[0][2] = -b * v - d;     L[0][3] = -b * w;             L[0][4] = b;
    L[1][0] = -2 * b * q * u;   L[1][1] = 2 * b * u * u + 1;  L[1][2] = 2 * b * v * u;  L[1][3] = 2 * b * w * u;      L[1][4] = -2 * b * u;
    L[2][0] = -2 * b * q + 1;   L[2][1] = 2 * b * u;          L[2][2] = 2 * b * v;      L[2][3] = 2 * b * w;          L[2][4] = -2 * b;
    L[3][0] = -2 * b * q * w;   L[3][1] = 2 * b * w * u;      L[3][2] = 2 * b * w * v;  L[3][3] = 2 * b * w * w + 1;  L[3][4] = -2 * b * w;
    L[4][0] = b * q - d * v;    L[4][1] = -b * u;             L[4][2] = -b * v + d;     L[4][3] = -b * w;             L[4][4] = b;
    return;
}
static void EigenvectorLX(const Real u, const Real v, const Real w, const Real c, 
        const Real q, const Real b, const Real d, Real L[restrict][DIMU])
{
    L[0][0] = b * q + d * u;   L[0][1] = -b * u - d;     L[0][2] = -b * v;             L[0][3] = -b * w;             L[0][4] = b;
    L[1][0] = -2 * b * q + 1;  L[1][1] = 2 * b * u;      L[1][2] = 2 * b * v;          L[1][3] = 2 * b * w;          L[1][4] = -2 * b;
    L[2][0] = -2 * b * q * v;  L[2][1] = 2 * b * v * u;  L[2][2] = 2 * b * v * v + 1;  L[2][3] = 2 * b * w * v;      L[2][4] = -2 * b * v;
    L[3][0] = -2 * b * q * w;  L[3][1] = 2 * b * w * u;  L[3][2] = 2 * b * w * v;      L[3][3] = 2 * b * w * w + 1;  L[3][4] = -2 * b * w;
    L[4][0] = b * q - d * u;   L[4][1] = -b * u + d;     L[4][2] = -b * v;             L[4][3] = -b * w;             L[4][4] = b;
    return;
}
void EigenvectorR(const int s, const Real Uo[restrict], Real R[restrict][DIMU])
{
    const Real u = Uo[1];
    const Real v = Uo[2];
    const Real w = Uo[3];
    const Real hT = Uo[4];
    const Real c = Uo[5];
    const Real q = 0.5 * (u * u + v * v + w * w);
    ComputeEigenvectorR[s](u, v, w, hT, c, q, R);
    return;
}
static void EigenvectorRZ(const Real u, const Real v, const Real w, const Real hT,
        const Real c, const Real q, Real R[restrict][DIMU])
{
    R[0][0] = 1;           R[0][1] = 0;  R[0][2] = 0;  R[0][3] = 1;          R[0][4] = 1;
    R[1][0] = u;           R[1][1] = 1;  R[1][2] = 0;  R[1][3] = 0;          R[1][4] = u;
    R[2][0] = v;           R[2][1] = 0;  R[2][2] = 1;  R[2][3] = 0;          R[2][4] = v;
    R[3][0] = w - c;       R[3][1] = 0;  R[3][2] = 0;  R[3][3] = w;          R[3][4] = w + c;
    R[4][0] = hT - w * c;  R[4][1] = u;  R[4][2] = v;  R[4][3] = w * w - q;  R[4][4] = hT + w * c;
    return;
}
static void EigenvectorRY(const Real u, const Real v, const Real w, const Real hT,
        const Real c, const Real q, Real R[restrict][DIMU])
{
    R[0][0] = 1;           R[0][1] = 0;  R[0][2] = 1;          R[0][3] = 0;  R[0][4] = 1;
    R[1][0] = u;           R[1][1] = 1;  R[1][2] = 0;          R[1][3] = 0;  R[1][4] = u;
    R[2][0] = v - c;       R[2][1] = 0;  R[2][2] = v;          R[2][3] = 0;  R[2][4] = v + c;
    R[3][0] = w;           R[3][1] = 0;  R[3][2] = 0;          R[3][3] = 1;  R[3][4] = w;
    R[4][0] = hT - v * c;  R[4][1] = u;  R[4][2] = v * v - q;  R[4][3] = w;  R[4][4] = hT + v * c;
    return;
}
static void EigenvectorRX(const Real u, const Real v, const Real w, const Real hT,
        const Real c, const Real q, Real R[restrict][DIMU])
{
    R[0][0] = 1;           R[0][1] = 1;          R[0][2] = 0;  R[0][3] = 0;  R[0][4] = 1;
    R[1][0] = u - c;       R[1][1] = u;          R[1][2] = 0;  R[1][3] = 0;  R[1][4] = u + c;
    R[2][0] = v;           R[2][1] = 0;          R[2][2] = 1;  R[2][3] = 0;  R[2][4] = v;
    R[3][0] = w;           R[3][1] = 0;          R[3][2] = 0;  R[3][3] = 1;  R[3][4] = w;
    R[4][0] = hT - u * c;  R[4][1] = u * u - q;  R[4][2] = v;  R[4][3] = w;  R[4][4] = hT + u * c;
    return;
}
void ConvectiveFlux(const int s, const Real gamma, const Real U[restrict], Real F[restrict])
{
    const Real rho = U[0];
    const Real u = U[1] / rho;
    const Real v = U[2] / rho;
    const Real w = U[3] / rho;
    const Real eT = U[4] / rho;
    const Real p = rho * (eT - 0.5 * (u * u + v * v + w * w)) * (gamma - 1.0);
    ComputeConvectiveFlux[s](rho, u, v, w, eT, p, F);
    return;
}
static void ConvectiveFluxZ(const Real rho, const Real u, const Real v, const Real w, 
        const Real eT, const Real p, Real F[restrict])
{
    F[0] = rho * w;
    F[1] = rho * w * u;
    F[2] = rho * w * v;
    F[3] = rho * w * w + p;
    F[4] = (rho * eT + p) * w;
    return;
}
static void ConvectiveFluxY(const Real rho, const Real u, const Real v, const Real w, 
        const Real eT, const Real p, Real F[restrict])
{
    F[0] = rho * v;
    F[1] = rho * v * u;
    F[2] = rho * v * v + p;
    F[3] = rho * v * w;
    F[4] = (rho * eT + p) * v;
    return;
}
static void ConvectiveFluxX(const Real rho, const Real u, const Real v, const Real w, 
        const Real eT, const Real p, Real F[restrict])
{
    F[0] = rho * u;
    F[1] = rho * u * u + p;
    F[2] = rho * u * v;
    F[3] = rho * u * w;
    F[4] = (rho * eT + p) * u;
    return;
}
void NumericalDiffusiveFlux(const int s, const int tn, const int k, const int j, 
        const int i, const int n[restrict], const Real dd[restrict], const Node *node, 
        const Model *model, Real Fvhat[restrict])
{
    DiffusiveFluxReconstructor ReconstructDiffusiveFlux[DIMS] = {
        NumericalDiffusiveFluxX,
        NumericalDiffusiveFluxY,
        NumericalDiffusiveFluxZ};
    ReconstructDiffusiveFlux[s](tn, k, j, i, n, dd, node, model, Fvhat);
    return;
}
static void NumericalDiffusiveFluxZ(const int tn, const int k, const int j, 
        const int i, const int n[restrict], const Real dd[restrict], const Node *node, 
        const Model *model, Real Fvhat[restrict])
{
    const int idx = IndexNode(k, j, i, n[Y], n[X]);
    const int idxW = IndexNode(k, j, i - 1, n[Y], n[X]);
    const int idxE = IndexNode(k, j, i + 1, n[Y], n[X]);
    const int idxS = IndexNode(k, j - 1, i, n[Y], n[X]);
    const int idxN = IndexNode(k, j + 1, i, n[Y], n[X]);

    const int idxB = IndexNode(k + 1, j, i, n[Y], n[X]);
    const int idxWB = IndexNode(k + 1, j, i - 1, n[Y], n[X]);
    const int idxEB = IndexNode(k + 1, j, i + 1, n[Y], n[X]);
    const int idxSB = IndexNode(k + 1, j - 1, i, n[Y], n[X]);
    const int idxNB = IndexNode(k + 1, j + 1, i, n[Y], n[X]);

    const Real * U = node[idx].U[tn];
    const Real u = U[1] / U[0];
    const Real v = U[2] / U[0];
    const Real w = U[3] / U[0];
    const Real T = ComputeTemperature(model->cv, U);

    U = node[idxW].U[tn];
    const Real uW = U[1] / U[0];
    const Real wW = U[3] / U[0];

    U = node[idxE].U[tn];
    const Real uE = U[1] / U[0];
    const Real wE = U[3] / U[0];

    U = node[idxS].U[tn];
    const Real vS = U[2] / U[0];
    const Real wS = U[3] / U[0];

    U = node[idxN].U[tn];
    const Real vN = U[2] / U[0];
    const Real wN = U[3] / U[0];

    U = node[idxB].U[tn];
    const Real uB = U[1] / U[0];
    const Real vB = U[2] / U[0];
    const Real wB = U[3] / U[0];
    const Real TB = ComputeTemperature(model->cv, U);

    U = node[idxWB].U[tn];
    const Real uWB = U[1] / U[0];
    const Real wWB = U[3] / U[0];

    U = node[idxEB].U[tn];
    const Real uEB = U[1] / U[0];
    const Real wEB = U[3] / U[0];

    U = node[idxSB].U[tn];
    const Real vSB = U[2] / U[0];
    const Real wSB = U[3] / U[0];

    U = node[idxNB].U[tn];
    const Real vNB = U[2] / U[0];
    const Real wNB = U[3] / U[0];

    const Real dw_dx = 0.25 * (wE + wEB - wW - wWB) * dd[X];
    const Real du_dz = (uB - u) * dd[Z];
    const Real dw_dy = 0.25 * (wN + wNB - wS - wSB) * dd[Y];
    const Real dv_dz = (vB - v) * dd[Z];
    const Real du_dx = 0.25 * (uE + uEB - uW - uWB) * dd[X];
    const Real dv_dy = 0.25 * (vN + vNB - vS - vSB) * dd[Y];
    const Real dw_dz = (wB - w) * dd[Z];
    const Real dT_dz = (TB - T) * dd[Z];

    /* Calculate interfacial values */
    const Real uhat = 0.5 * (u + uB);
    const Real vhat = 0.5 * (v + vB);
    const Real what = 0.5 * (w + wB);
    const Real That = 0.5 * (T + TB);
    const Real mu = model->refMu * Viscosity(That * model->refT);
    const Real heatK = model->gamma * model->cv * mu / PrandtlNumber();
    const Real divV = du_dx + dv_dy + dw_dz;

    Fvhat[0] = 0;
    Fvhat[1] = mu * (dw_dx + du_dz);
    Fvhat[2] = mu * (dw_dy + dv_dz);
    Fvhat[3] = mu * (2.0 * dw_dz - (2.0/3.0) * divV);
    Fvhat[4] = heatK * dT_dz + Fvhat[1] * uhat + Fvhat[2] * vhat + Fvhat[3] * what;
    return;
}
static void NumericalDiffusiveFluxY(const int tn, const int k, const int j, 
        const int i, const int n[restrict], const Real dd[restrict], const Node *node, 
        const Model *model, Real Fvhat[restrict])
{
    const int idx = IndexNode(k, j, i, n[Y], n[X]);
    const int idxW = IndexNode(k, j, i - 1, n[Y], n[X]);
    const int idxE = IndexNode(k, j, i + 1, n[Y], n[X]);
    const int idxF = IndexNode(k - 1, j, i, n[Y], n[X]);
    const int idxB = IndexNode(k + 1, j, i, n[Y], n[X]);

    const int idxN = IndexNode(k, j + 1, i, n[Y], n[X]);
    const int idxWN = IndexNode(k, j + 1, i - 1, n[Y], n[X]);
    const int idxEN = IndexNode(k, j + 1, i + 1, n[Y], n[X]);
    const int idxFN = IndexNode(k - 1, j + 1, i, n[Y], n[X]);
    const int idxBN = IndexNode(k + 1, j + 1, i, n[Y], n[X]);

    const Real * U = node[idx].U[tn];
    const Real u = U[1] / U[0];
    const Real v = U[2] / U[0];
    const Real w = U[3] / U[0];
    const Real T = ComputeTemperature(model->cv, U);

    U = node[idxW].U[tn];
    const Real uW = U[1] / U[0];
    const Real vW = U[2] / U[0];

    U = node[idxE].U[tn];
    const Real uE = U[1] / U[0];
    const Real vE = U[2] / U[0];

    U = node[idxF].U[tn];
    const Real vF = U[2] / U[0];
    const Real wF = U[3] / U[0];

    U = node[idxB].U[tn];
    const Real vB = U[2] / U[0];
    const Real wB = U[3] / U[0];

    U = node[idxN].U[tn];
    const Real uN = U[1] / U[0];
    const Real vN = U[2] / U[0];
    const Real wN = U[3] / U[0];
    const Real TN = ComputeTemperature(model->cv, U);

    U = node[idxWN].U[tn];
    const Real uWN = U[1] / U[0];
    const Real vWN = U[2] / U[0];

    U = node[idxEN].U[tn];
    const Real uEN = U[1] / U[0];
    const Real vEN = U[2] / U[0];

    U = node[idxFN].U[tn];
    const Real vFN = U[2] / U[0];
    const Real wFN = U[3] / U[0];

    U = node[idxBN].U[tn];
    const Real vBN = U[2] / U[0];
    const Real wBN = U[3] / U[0];

    const Real dv_dx = 0.25 * (vE + vEN - vW - vWN) * dd[X];
    const Real du_dy = (uN - u) * dd[Y];
    const Real dv_dy = (vN - v) * dd[Y];
    const Real du_dx = 0.25 * (uE + uEN - uW - uWN) * dd[X];
    const Real dw_dz = 0.25 * (wB + wBN - wF - wFN) * dd[Z];
    const Real dv_dz = 0.25 * (vB + vBN - vF - vFN) * dd[Z];
    const Real dw_dy = (wN - w) * dd[Y];
    const Real dT_dy = (TN - T) * dd[Y];

    /* Calculate interfacial values */
    const Real uhat = 0.5 * (u + uN);
    const Real vhat = 0.5 * (v + vN);
    const Real what = 0.5 * (w + wN);
    const Real That = 0.5 * (T + TN);
    const Real mu = model->refMu * Viscosity(That * model->refT);
    const Real heatK = model->gamma * model->cv * mu / PrandtlNumber();
    const Real divV = du_dx + dv_dy + dw_dz;

    Fvhat[0] = 0;
    Fvhat[1] = mu * (dv_dx + du_dy);
    Fvhat[2] = mu * (2.0 * dv_dy - (2.0/3.0) * divV);
    Fvhat[3] = mu * (dv_dz + dw_dy);
    Fvhat[4] = heatK * dT_dy + Fvhat[1] * uhat + Fvhat[2] * vhat + Fvhat[3] * what;
    return ;
}
static void NumericalDiffusiveFluxX(const int tn, const int k, const int j, 
        const int i, const int n[restrict], const Real dd[restrict], const Node *node, 
        const Model *model, Real Fvhat[restrict])
{
    const int idx = IndexNode(k, j, i, n[Y], n[X]);
    const int idxS = IndexNode(k, j - 1, i, n[Y], n[X]);
    const int idxN = IndexNode(k, j + 1, i, n[Y], n[X]);
    const int idxF = IndexNode(k - 1, j, i, n[Y], n[X]);
    const int idxB = IndexNode(k + 1, j, i, n[Y], n[X]);

    const int idxE = IndexNode(k, j, i + 1, n[Y], n[X]);
    const int idxSE = IndexNode(k, j - 1, i + 1, n[Y], n[X]);
    const int idxNE = IndexNode(k, j + 1, i + 1, n[Y], n[X]);
    const int idxFE = IndexNode(k - 1, j, i + 1, n[Y], n[X]);
    const int idxBE = IndexNode(k + 1, j, i + 1, n[Y], n[X]);

    const Real * U = node[idx].U[tn];
    const Real u = U[1] / U[0];
    const Real v = U[2] / U[0];
    const Real w = U[3] / U[0];
    const Real T = ComputeTemperature(model->cv, U);

    U = node[idxS].U[tn];
    const Real uS = U[1] / U[0];
    const Real vS = U[2] / U[0];

    U = node[idxN].U[tn];
    const Real uN = U[1] / U[0];
    const Real vN = U[2] / U[0];

    U = node[idxF].U[tn];
    const Real uF = U[1] / U[0];
    const Real wF = U[3] / U[0];

    U = node[idxB].U[tn];
    const Real uB = U[1] / U[0];
    const Real wB = U[3] / U[0];

    U = node[idxE].U[tn];
    const Real uE = U[1] / U[0];
    const Real vE = U[2] / U[0];
    const Real wE = U[3] / U[0];
    const Real TE = ComputeTemperature(model->cv, U);

    U = node[idxSE].U[tn];
    const Real uSE = U[1] / U[0];
    const Real vSE = U[2] / U[0];

    U = node[idxNE].U[tn];
    const Real uNE = U[1] / U[0];
    const Real vNE = U[2] / U[0];

    U = node[idxFE].U[tn];
    const Real uFE = U[1] / U[0];
    const Real wFE = U[3] / U[0];

    U = node[idxBE].U[tn];
    const Real uBE = U[1] / U[0];
    const Real wBE = U[3] / U[0];

    const Real du_dx = (uE - u) * dd[X];
    const Real dv_dy = 0.25 * (vN + vNE - vS - vSE) * dd[Y];
    const Real dw_dz = 0.25 * (wB + wBE - wF - wFE) * dd[Z];
    const Real du_dy = 0.25 * (uN + uNE - uS - uSE) * dd[Y];
    const Real dv_dx = (vE - v) * dd[X];
    const Real du_dz = 0.25 * (uB + uBE - uF - uFE) * dd[Z];
    const Real dw_dx = (wE - w) * dd[X];
    const Real dT_dx = (TE - T) * dd[X];

    /* Calculate interfacial values */
    const Real uhat = 0.5 * (u + uE);
    const Real vhat = 0.5 * (v + vE);
    const Real what = 0.5 * (w + wE);
    const Real That = 0.5 * (T + TE);
    const Real mu = model->refMu * Viscosity(That * model->refT);
    const Real heatK = model->gamma * model->cv * mu / PrandtlNumber();
    const Real divV = du_dx + dv_dy + dw_dz;

    Fvhat[0] = 0;
    Fvhat[1] = mu * (2.0 * du_dx - (2.0/3.0) * divV);
    Fvhat[2] = mu * (du_dy + dv_dx);
    Fvhat[3] = mu * (du_dz + dw_dx);
    Fvhat[4] = heatK * dT_dx + Fvhat[1] * uhat + Fvhat[2] * vhat + Fvhat[3] * what;
    return;
}
static Real Viscosity(const Real T)
{
    return 1.458e-6 * pow(T, 1.5) / (T + 110.4); /* Sutherland's law */
}
static Real PrandtlNumber(void)
{
    return 0.71; /* air */
}
/*
 * Get value of primitive variable vector.
 */
void PrimitiveByConservative(const Real gamma, const Real gasR, const Real U[restrict], Real Uo[restrict])
{
    Uo[0] = U[0];
    Uo[1] = U[1] / U[0];
    Uo[2] = U[2] / U[0];
    Uo[3] = U[3] / U[0];
    Uo[4] = (U[4] - 0.5 * (U[1] * U[1] + U[2] * U[2] + U[3] * U[3]) / U[0]) * (gamma - 1.0);
    Uo[5] = Uo[4] / (Uo[0] * gasR);
    return;
}
Real ComputePressure(const Real gamma, const Real U[restrict])
{
    return (U[4] - 0.5 * (U[1] * U[1] + U[2] * U[2] + U[3] * U[3]) / U[0]) * (gamma - 1.0);
}
Real ComputeTemperature(const Real cv, const Real U[restrict])
{
    return (U[4] - 0.5 * (U[1] * U[1] + U[2] * U[2] + U[3] * U[3]) / U[0]) / (U[0] * cv);
}
/*
 * Compute conservative variable vector according to primitives.
 */
void ConservativeByPrimitive(const Real gamma, const Real Uo[restrict], Real U[restrict])
{
    U[0] = Uo[0];
    U[1] = Uo[0] * Uo[1];
    U[2] = Uo[0] * Uo[2];
    U[3] = Uo[0] * Uo[3];
    U[4] = 0.5 * Uo[0] * (Uo[1] * Uo[1] + Uo[2] * Uo[2] + Uo[3] * Uo[3]) + Uo[4] / (gamma - 1.0); 
    return;
}
/*
 * Index math.
 */
int IndexNode(const int k, const int j, const int i, const int jMax, const int iMax)
{
    return (k * jMax + j) * iMax + i;
}
/*
 * Coordinates transformations.
 * When transform from spatial coordinates to node coordinates, a half
 * grid distance shift is necessary to ensure obtaining a closest node
 * coordinates considering the downward truncation of (int). 
 * Note: current rounding conversion only works for positive float.
 */
int NodeSpace(const Real s, const Real sMin, const Real dds, const int ng)
{
    return (int)((s - sMin) * dds + 0.5) + ng;
}
int ValidNodeSpace(const int n, const int nMin, const int nMax)
{
    return MinInt(nMax - 1, MaxInt(nMin, n));
}
Real PointSpace(const int n, const Real sMin, const Real ds, const int ng)
{
    return sMin + (n - ng) * ds;
}
/*
 * Math functions
 */
Real MinReal(const Real x, const Real y)
{
    if (x < y) {
        return x;
    }
    return y;
}
Real MaxReal(const Real x, const Real y)
{
    if (x > y) {
        return x;
    }
    return y;
}
int MinInt(const int x, const int y)
{
    if (x < y) {
        return x;
    }
    return y;
}
int MaxInt(const int x, const int y)
{
    if (x > y) {
        return x;
    }
    return y;
}
int Sign(const Real x)
{
    if (0 < x) {
        return 1;
    }
    if (0 > x) {
        return -1;
    }
    return 0;
}
Real Dot(const Real V1[], const Real V2[])
{
    return (V1[X] * V2[X] + V1[Y] * V2[Y] + V1[Z] * V2[Z]);
}
Real Norm(const Real V[])
{
    return sqrt(Dot(V, V));
}
Real Dist2(const Real V1[], const Real V2[])
{
    const RealVector V = {V1[X] - V2[X], V1[Y] - V2[Y], V1[Z] - V2[Z]};
    return Dot(V, V);
}
Real Dist(const Real V1[], const Real V2[])
{
    return sqrt(Dist2(V1, V2));
}
void Cross(const Real V1[restrict], const Real V2[restrict], Real V[restrict])
{
    V[X] = V1[Y] * V2[Z] - V1[Z] * V2[Y];
    V[Y] = V1[Z] * V2[X] - V1[X] * V2[Z];
    V[Z] = V1[X] * V2[Y] - V1[Y] * V2[X];
    return;
}
void OrthogonalSpace(const Real N[restrict], Real Ta[restrict], Real Tb[restrict])
{
    int mark = Z; /* default mark for minimum component */
    if (fabs(N[mark]) > fabs(N[Y])) {
        mark = Y;
    }
    if (fabs(N[mark]) > fabs(N[X])) {
        mark = X;
    }
    if (X == mark) {
        Ta[X] = 0;
        Ta[Y] = -N[Z];
        Ta[Z] = N[Y];
    } else {
        if (Y == mark) {
            Ta[X] = -N[Z];
            Ta[Y] = 0;
            Ta[Z] = N[X];
        } else {
            Ta[X] = -N[Y];
            Ta[Y] = N[X];
            Ta[Z] = 0;
        }
    }
    Normalize(DIMS, Norm(Ta), Ta);
    Cross(Tb, N, Ta);
    return;
}
void Normalize(const int dimV, const Real normalizer, Real V[])
{
    for (int n = 0; n < dimV; ++n) {
        V[n] = V[n] / normalizer;
    }
    return;
}
/* a good practice: end file with a newline */


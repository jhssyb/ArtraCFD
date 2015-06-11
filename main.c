/****************************************************************************
 *                              ArtraCFD                                    *
 *                          <By Huangrui Mo>                                *
 * Copyright (C) 2014-2018 Huangrui Mo <huangrui.mo@gmail.com>              *
 * This file is part of ArtraCFD.                                           *
 * ArtraCFD is free software: you can redistribute it and/or modify it      *
 * under the terms of the GNU General Public License as published by        *
 * the Free Software Foundation, either version 3 of the License, or        *
 * (at your option) any later version.                                      *
 ****************************************************************************/
/****************************************************************************
 * Required Header Files
 ****************************************************************************/
#include "commons.h"
#include "program_entrance.h"
#include "preprocess.h"
#include "solve.h"
#include "postprocess.h"
#include <stdio.h> /* standard library for input and output */
#include <stdlib.h> /* dynamic memory allocation and exit */
#include <string.h> /* manipulating strings */
/****************************************************************************
 * The Main Function
 ****************************************************************************/
int main(int argc, char *argv[])
{
    /*
     * Declare and initialize variables
     */    
    Field theField = { /* flow field variables */
        .Un = NULL,
        .U = NULL,
        .Uswap = NULL};
    Space theSpace = { /* space dimensions */
        .nz = 0,
        .ny = 0,
        .nx = 0,
        .ng = 0,
        .kMax = 0,
        .jMax = 0,
        .iMax = 0,
        .nMax = 0,
        .collapsed = 0,
        .dz = 0.0,
        .dy = 0.0,
        .dx = 0.0,
        .ddz = 0.0,
        .ddy = 0.0,
        .ddx = 0.0,
        .tinyL = 0.0,
        .zMin = 0.0,
        .yMin = 0.0,
        .xMin = 0.0,
        .zMax = 0.0,
        .yMax = 0.0,
        .xMax = 0.0,
        .nodeFlag = NULL};
    Geometry theGeometry = { /* geometry entities */
        .totalN = 0,
        .headAddress = NULL};
    Time theTime = { /* time dimensions */
        .restart = 0,
        .totalTime = 0.0,
        .currentTime = 0.0,
        .dt = 0.0,
        .numCFL = 0.0,
        .totalStep = 0,
        .stepCount = 0,
        .totalOutputTimes = 0,
        .outputCount = 0,
        .dataStreamer = 0};
    Flow theFlow = { /* flow parameters */
        .refMa = 0.0,
        .refMu = 0.0,
        .refPr = 0.0,
        .gamma = 0.0,
        .gasR = 0.0,
        .cv = 0.0,
        .pi = 0.0,
        .delta = 0.0,
        .refLength = 0.0,
        .refDensity = 0.0,
        .refVelocity = 0.0,
        .refTemperature = 0.0,
        .outputProbe = 0,
        .tallyProbe = 0,
        .probe = {{0.0}}};
    Partition thePart = { /* domain partition control */
        .totalN = 1,
        .kSub = {0},
        .kSup = {0},
        .jSub = {0},
        .jSup = {0},
        .iSub = {0},
        .iSup = {0},
        .normalZ = {0},
        .normalY = {0},
        .normalX = {0},
        .typeBC = {0},
        .valueBC = {{0.0}},
        .tallyIC = 0,
        .typeIC = {0},
        .valueIC = {{0.0}}};
    Control theControl = { /* program overall control */
        .runMode = 'i',
        .processorN = 1};
    /*
     * Program Entrance
     */
    ProgramEntrance(argc, argv, &theControl);
    /*
     * Preprocessing
     */
    Preprocess(&theField, &theSpace, &theGeometry, &theTime, &thePart, &theFlow);
    /*
     * Solve
     */
    Solve(&theField, &theSpace, &theGeometry, &theTime, &thePart, &theFlow);
    /*
     * Postprocessing
     */
    Postprocess(&theField, &theSpace, &theGeometry);
    /*
     * Successfully return
     */
    exit(EXIT_SUCCESS); /* exiting program */ 
}
/* a good practice: end file with a newline */


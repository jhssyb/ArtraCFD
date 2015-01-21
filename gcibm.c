/****************************************************************************
 * Space Domain Meshing                                                     *
 * Last-modified: 18 Jan 2015 10:08:54 PM
 * Programmer: Huangrui Mo                                                  *
 * - Follow the Google's C/C++ style Guide.                                 *
 * - This file use ghost cell immersed boundary method to handle complex    *
 *   geometries                                                             *
 ****************************************************************************/
/****************************************************************************
 * Required Header Files
 ****************************************************************************/
#include "gcibm.h"
#include <stdio.h> /* standard library for input and output */
#include <stdlib.h> /* dynamic memory allocation and exit */
#include <math.h> /* common mathematical functions */
#include <string.h> /* manipulating strings */
#include "commons.h"
/****************************************************************************
 * Static Function Declarations
 ****************************************************************************/
static int LocateSolidGeometry(Space *, Particle *, const Partition *);
static int IdentifyGhostCells(Space *, const Partition *);
/****************************************************************************
 * Function definitions
 ****************************************************************************/
/*
 * These functions identify the type of each node: 
 * boundary and exterior ghost cell (2), interior ghost cell (1), 
 * interior solid cell (-1), interior fluid cells (0),
 * The procedures are: first, initialize ghostFlag of all nodes to type (2);
 * then, identify all inner nodes as in solid geometry (-1) or fluid (0);
 * finally, identify ghost nodes according to its neighbours' node type;
 * moreover, whenever identifying a solid node or ghost node, store its
 * corresponding geometry information by linking the node to the corresponding
 * geometry ID, which will be accessed to calculate other informations. 
 * The rational is that don't store every information for each ghost cell, but
 * only store necessary information. When need it, access and calculate it.
 */
/*
 * This function initializes the geometry for the entire domain.
 * It's separated because only need to be executed once as initialization.
 */
int InitializeDomainGeometryGCIBM(Space *space, Particle *particle, const Partition *part)
{
    ShowInformation("Initialize domain geometry...");
    /*
     * first initialize the entire domain to type "2"
     */
    /* linear array index math variable */
    int idx = 0; /* calculated index */
    /* for loop control */
    int k = 0; /* count, k for z dimension */
    int j = 0; /* count, j for y dimension */
    int i = 0; /* count, i for x dimension */
    for (k = 0; k < space->kMax; ++k) {
        for (j = 0; j < space->jMax; ++j) {
            for (i = 0; i < space->iMax; ++i) {
                idx = (k * space->jMax + j) * space->iMax + i;
                space->ghostFlag[idx] = 2;
            }
        }
    }
    /*
     * then call ComputeDomainGeometryGCIBM
     */
    ComputeDomainGeometryGCIBM(space, particle, part); /* domain meshing */
    ShowInformation("Session End");
    return 0;
}
int ComputeDomainGeometryGCIBM(Space *space, Particle *particle, const Partition *part)
{
    ShowInformation("  Computing domain geometry...");
    LocateSolidGeometry(space, particle, part);
    IdentifyGhostCells(space, part);
    return 0;
}
static int LocateSolidGeometry(Space *space, Particle *particle, const Partition *part)
{
    ShowInformation("    Locate solid geometry...");
    /* linear array index math variable */
    int idx = 0; /* calculated index */
    /* for loop control */
    int k = 0; /* count, k for z dimension */
    int j = 0; /* count, j for y dimension */
    int i = 0; /* count, i for x dimension */
    /* geometry computation */
    int geoCount = 0; /* geometry objects count */
    double distance = 0;
    double distX = 0;
    double distY = 0;
    double distZ = 0;
    double radius = 0;
    for (k = part->kSub[12]; k < part->kSup[12]; ++k) {
        for (j = part->jSub[12]; j < part->jSup[12]; ++j) {
            for (i = part->iSub[12]; i < part->iSup[12]; ++i) {
                idx = (k * space->jMax + j) * space->iMax + i;
                space->ghostFlag[idx] = 0; /* reset to fluid */
                for (geoCount = 0; geoCount < particle->totalN; ++geoCount) {
                    distX = pow((i - space->ng) * space->dx - particle->x[geoCount], 2);
                    distY = pow((j - space->ng) * space->dy - particle->y[geoCount], 2);
                    distZ = pow((k - space->ng) * space->dz - particle->z[geoCount], 2);
                    radius = pow(particle->r[geoCount], 2);
                    distance = distX + distY + distZ - radius;
                    if (distance < 0) { /* in the solid geometry */
                        space->ghostFlag[idx] = -1;
                        space->geoID[idx] = geoCount;
                    }
                }
            }
        }
    }
    return 0;
}
static int IdentifyGhostCells(Space *space, const Partition *part)
{
    ShowInformation("    Identify ghost cells...");
    /* linear array index math variable */
    int idx = 0; /* calculated index */
    int idxW = 0; /* index at West */
    int idxE = 0; /* index at East */
    int idxS = 0; /* index at South */
    int idxN = 0; /* index at North */
    int idxF = 0; /* index at Front */
    int idxB = 0; /* index at Back */
    /* for loop control */
    int k = 0; /* count, k for z dimension */
    int j = 0; /* count, j for y dimension */
    int i = 0; /* count, i for x dimension */
    /* criteria */
    int flag = 0;
    for (k = part->kSub[12]; k < part->kSup[12]; ++k) {
        for (j = part->jSub[12]; j < part->jSup[12]; ++j) {
            for (i = part->iSub[12]; i < part->iSup[12]; ++i) {
                idx = (k * space->jMax + j) * space->iMax + i;
                if (space->ghostFlag[idx] == -1) {
                    idxW = (k * space->jMax + j) * space->iMax + i - 1;
                    idxE = (k * space->jMax + j) * space->iMax + i + 1;
                    idxS = (k * space->jMax + j - 1) * space->iMax + i;
                    idxN = (k * space->jMax + j + 1) * space->iMax + i;
                    idxF = ((k - 1) * space->jMax + j) * space->iMax + i;
                    idxB = ((k + 1) * space->jMax + j) * space->iMax + i;
                    flag = space->ghostFlag[idxW] * space->ghostFlag[idxE] * 
                        space->ghostFlag[idxS] * space->ghostFlag[idxN] * 
                        space->ghostFlag[idxF] * space->ghostFlag[idxB];
                    if (flag == 0) { /* if exist one neighbour is fluid, then it's ghost */
                        space->ghostFlag[idx] = 1;
                    }

                }
            }
        }
    }
    return 0;
}
/* a good practice: end file with a newline */

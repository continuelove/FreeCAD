/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>

# include <BRepMesh_IncrementalMesh.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
#endif

#include <Base/Console.h>
#include <Base/Sequencer.h>
#include <Mod/Part/App/Tools.h>

#include "PovTools.h"
#include "LuxTools.h"


using Base::Console;
using namespace Raytracing;

std::string LuxTools::getCamera(const CamDef& Cam)
{
    std::stringstream out;
    out << "# declares position and view direction" << std::endl
        << "# Generated by FreeCAD (http://www.freecad.org/)"
        << std::endl

        // writing Camera positions
        << "LookAt " << Cam.CamPos.X() << " " << Cam.CamPos.Y() << " " << Cam.CamPos.Z() << " "
        // writing lookat
        << Cam.LookAt.X()  << " " << Cam.LookAt.Y() << " " << Cam.LookAt.Z() << " "
        // writing the Up Vector
        << Cam.Up.X() << " " << Cam.Up.Y() << " " << Cam.Up.Z() << std::endl;

    return out.str();
}

void LuxTools::writeShape(std::ostream &out, const char *PartName, const TopoDS_Shape& Shape, float fMeshDeviation)
{
    Base::Console().Log("Meshing with Deviation: %f\n",fMeshDeviation);

    TopExp_Explorer ex;
    BRepMesh_IncrementalMesh MESH(Shape,fMeshDeviation);

    // counting faces and start sequencer
    int l = 1;
    for (ex.Init(Shape, TopAbs_FACE); ex.More(); ex.Next(),l++) {}
    Base::SequencerLauncher seq("Writing file", l);
    
    // write object
    out << "AttributeBegin #  \"" << PartName << "\"" << std::endl;
    out << "Transform [1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1]" << std::endl;
    out << "NamedMaterial \"FreeCADMaterial_" << PartName << "\"" << std::endl;
    out << "Shape \"mesh\"" << std::endl;
    
    // gather vertices, normals and face indices
    std::stringstream triindices;
    std::stringstream N;
    std::stringstream P;
    l = 1;
    int vi = 0;
    for (ex.Init(Shape, TopAbs_FACE); ex.More(); ex.Next(),l++) {

        // get the shape and mesh it
        const TopoDS_Face& aFace = TopoDS::Face(ex.Current());

        std::vector<gp_Pnt> points;
        std::vector<gp_Vec> vertexnormals;
        std::vector<Poly_Triangle> facets;
        if (!Part::Tools::getTriangulation(aFace, points, facets)) {
            break;
        }

        Part::Tools::getPointNormals(points, facets, vertexnormals);
        Part::Tools::getPointNormals(points, aFace, vertexnormals);

        // writing vertices
        for (std::size_t i=0; i < points.size(); i++) {
            P << points[i].X() << " " << points[i].Y() << " " << points[i].Z() << " ";
        }

        // writing per vertex normals
        for (std::size_t j=0; j < vertexnormals.size(); j++) {
            N << vertexnormals[j].X() << " "  << vertexnormals[j].Y() << " " << vertexnormals[j].Z() << " ";
        }

        // writing triangle indices
        for (std::size_t k=0; k < facets.size(); k++) {
            Standard_Integer n1, n2, n3;
            facets[k].Get(n1, n2, n3);
            triindices << n1 + vi << " " << n3 + vi << " " << n2 + vi << " ";
        }
        
        vi = vi + points.size();

        seq.next();

    } // end of face loop

    // write mesh data
    out << "    \"integer triindices\" [" << triindices.str() << "]" << std::endl;
    out << "    \"point P\" [" << P.str() << "]" << std::endl;
    out << "    \"normal N\" [" << N.str() << "]" << std::endl;
    out << "    \"bool generatetangents\" [\"false\"]" << std::endl;
    out << "    \"string name\" [\"" << PartName << "\"]" << std::endl;
    out << "AttributeEnd # \"\"" << std::endl;
}

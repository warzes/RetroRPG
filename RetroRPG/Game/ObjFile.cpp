#include "ObjFile.h"
#include "ImageCache.h"
//-----------------------------------------------------------------------------
ObjMaterial defMaterial;
//-----------------------------------------------------------------------------
ObjFile::ObjFile(const char* filename) 
	: m_curMaterial(&defMaterial)
{
	memset(m_line, 0, OBJ_MAX_LINE + 1);
	if( filename ) m_path = _strdup(filename);
}
//-----------------------------------------------------------------------------
ObjFile::~ObjFile()
{
	free(m_path);
	delete[] m_materials;
}
//-----------------------------------------------------------------------------
Mesh* ObjFile::Load(int index)
{
	FILE* file = fopen(m_path, "rb");
	if( !file ) return NULL;
	if( !m_materials )	loadMaterialLibraries(file);

	Mesh* mesh = new Mesh();

	fseek(file, 0, SEEK_SET);
	int noObjects = 0;
	int len = readLine(file);
	while( len )
	{
		if( strncmp(m_line, "o ", 2) == 0 )
		{
			if( noObjects++ == index )
			{
				strncpy(mesh->name, &m_line[2], OBJ_MAX_NAME);
				mesh->name[OBJ_MAX_NAME] = '\0';
				importMeshAllocate(file, mesh);
				importMeshData(file, mesh);
				break;
			}
		}
		len = readLine(file);
	}

	fclose(file);
	return mesh;
}
//-----------------------------------------------------------------------------
void ObjFile::Save(const Mesh* mesh)
{
	FILE* objFile = fopen(m_path, "wb");
	if( !objFile ) return;

	char mtlName[MAX_FILE_NAME + 1];
	mtlName[0] = '\0';
	char mtlPath[MAX_FILE_PATH + 1];
	mtlPath[0] = '\0';

	Global::getFileName(mtlName, MAX_FILE_NAME, m_path);
	Global::getFileDirectory(mtlPath, MAX_FILE_PATH, m_path);
	int len = strlen(mtlName);
	if( len < 3 )
	{
		fclose(objFile);
		return;
	}
	mtlName[len - 3] = 'm';
	mtlName[len - 2] = 't';
	mtlName[len - 1] = 'l';
	strcat(mtlPath, "/");
	strcat(mtlPath, mtlName);

	FILE * mtlFile = fopen(mtlPath, "wb");
	if( !mtlFile )
	{
		fclose(objFile);
		return;
	}

	fseek(mtlFile, 0, SEEK_SET);
	exportHeader(mtlFile, mesh);
	exportMaterials(mtlFile, mesh);

	fseek(objFile, 0, SEEK_SET);
	exportHeader(objFile, mesh);
	fprintf(objFile, "mtllib %s\n", mtlName);
	fprintf(objFile, "o %s\n", mesh->name);
	exportVertexes(objFile, mesh);
	exportTexCoords(objFile, mesh);
	exportTriangles(objFile, mesh);

	fclose(mtlFile);
	fclose(objFile);
}
//-----------------------------------------------------------------------------
void ObjFile::exportHeader(FILE* file, const Mesh* mesh)
{
	fprintf(file, "# RetroRPG - ObjFile exporter\n");
}
//-----------------------------------------------------------------------------
void ObjFile::exportMaterials(FILE* file, const Mesh* mesh)
{
	fprintf(file, "newmtl Default\n");
	fprintf(file, "Ns 0.8\n");
	fprintf(file, "Ka 1.0 1.0 1.0\n");
	fprintf(file, "Kd 1.0 1.0 1.0\n");
	fprintf(file, "Ks 0.5 0.5 0.5\n\n");

	if( !mesh->texSlotList ) return;

	for( int i = 0; i < mesh->noTriangles; i++ )
	{
		bool texNew = true;
		int tex = mesh->texSlotList[i];
		for( int j = 0; j < i; j++ )
		{
			if( mesh->texSlotList[j] == tex )
			{
				texNew = false;
				break;
			}
		}
		if( !texNew ) continue;

		char * name = gImageCache.cacheSlots[tex].name;
		fprintf(file, "newmtl %s\n", name);
		fprintf(file, "Ns 0.8\n");
		fprintf(file, "Ka 1.0 1.0 1.0\n");
		fprintf(file, "Kd 1.0 1.0 1.0\n");
		fprintf(file, "Ks 0.5 0.5 0.5\n");
		fprintf(file, "map_Kd %s\n\n", name);
	}
}
//-----------------------------------------------------------------------------
void ObjFile::exportVertexes(FILE* file, const Mesh* mesh)
{
	if( !mesh->vertexes ) return;
	for( int i = 0; i < mesh->noVertexes; i++ )
		fprintf(file, "v %f %f %f\n", mesh->vertexes[i].x, mesh->vertexes[i].y, mesh->vertexes[i].z);
	fprintf(file, "\n");
}
//-----------------------------------------------------------------------------
void ObjFile::exportTexCoords(FILE* file, const Mesh* mesh)
{
	if( !mesh->texCoords ) return;
	for( int i = 0; i < mesh->noTexCoords; i++ )
		fprintf(file, "vt %f %f\n", mesh->texCoords[i * 2 + 0], mesh->texCoords[i * 2 + 1]);
	fprintf(file, "\n");
}
//-----------------------------------------------------------------------------
void ObjFile::exportNormals(FILE* file, const Mesh* mesh)
{
	if( !mesh->normals ) return;
	for( int i = 0; i < mesh->noVertexes; i++ )
		fprintf(file, "vn %f %f %f\n", mesh->normals[i].x, mesh->normals[i].y, mesh->normals[i].z);
	fprintf(file, "\n");
}
//-----------------------------------------------------------------------------
void ObjFile::exportTriangles(FILE* file, const Mesh* mesh)
{
	if( !mesh->vertexesList ) return;
	if( !mesh->texCoordsList ) return;
	if( !mesh->texSlotList ) return;

	int texLast = -1;
	for( int i = 0; i < mesh->noTriangles; i++ )
	{
		int tex = mesh->texSlotList[i];
		if( texLast != tex )
		{
			if( !tex )
			{
				fprintf(file, "usemtl Default\n");
			}
			else
			{
				char * name = gImageCache.cacheSlots[tex].name;
				fprintf(file, "usemtl %s\n", name);
			}
			fprintf(file, "s off\n");
			texLast = tex;
		}

		if( mesh->texCoordsList && mesh->normals )
		{
			fprintf(file, "f %i/%i/%i %i/%i/%i %i/%i/%i\n",
				mesh->vertexesList[i * 3 + 0] + 1, mesh->texCoordsList[i * 3 + 0] + 1, i * 3 + 1,
				mesh->vertexesList[i * 3 + 1] + 1, mesh->texCoordsList[i * 3 + 1] + 1, i * 3 + 2,
				mesh->vertexesList[i * 3 + 2] + 1, mesh->texCoordsList[i * 3 + 2] + 1, i * 3 + 3
			);
		}
		else if( mesh->texCoordsList )
		{
			fprintf(file, "f %i/%i %i/%i %i/%i\n",
				mesh->vertexesList[i * 3 + 0] + 1, mesh->texCoordsList[i * 3 + 0] + 1,
				mesh->vertexesList[i * 3 + 1] + 1, mesh->texCoordsList[i * 3 + 1] + 1,
				mesh->vertexesList[i * 3 + 2] + 1, mesh->texCoordsList[i * 3 + 2] + 1
			);
		}
		else if( mesh->normals )
		{
			fprintf(file, "f %i//%i %i//%i %i//%i\n",
				mesh->vertexesList[i * 3 + 0] + 1, i * 3 + 1,
				mesh->vertexesList[i * 3 + 1] + 1, i * 3 + 2,
				mesh->vertexesList[i * 3 + 2] + 1, i * 3 + 3
			);
		}
		else
		{
			fprintf(file, "f %i %i %i\n",
				mesh->vertexesList[i * 3 + 0] + 1,
				mesh->vertexesList[i * 3 + 1] + 1,
				mesh->vertexesList[i * 3 + 2] + 1
			);
		}
	}
	fprintf(file, "\n");
}
//-----------------------------------------------------------------------------
int ObjFile::GetNoMeshes()
{
	FILE* file = fopen(m_path, "rb");
	if( !file ) return 0;

	int noObjects = 0;
	int len = readLine(file);
	while( len )
	{
		if( strncmp(m_line, "o ", 2) == 0 ) noObjects++;
		len = readLine(file);
	}

	fclose(file);
	return noObjects;
}
//-----------------------------------------------------------------------------
const char* ObjFile::GetMeshName(int index)
{
	FILE* file = fopen(m_path, "rb");
	if( !file ) return NULL;

	int noObjects = 0;
	int len = readLine(file);
	while( len )
	{
		if( strncmp(m_line, "o ", 2) == 0 )
		{
			if( noObjects++ == index )
			{
				fclose(file);
				return &m_line[2];
			}
		}
		len = readLine(file);
	}

	fclose(file);
	return NULL;
}
//-----------------------------------------------------------------------------
void ObjFile::loadMaterialLibraries(FILE* file)
{
	char filename[OBJ_MAX_PATH + 1];
	char libName[OBJ_MAX_PATH + OBJ_MAX_NAME + 1];

	m_noMaterials = 0;

	fseek(file, 0, SEEK_SET);
	int len = readLine(file);
	while( len )
	{
		if( strncmp(m_line, "mtllib ", 7) == 0 )
		{
			strncpy(libName, &m_line[7], OBJ_MAX_NAME);
			libName[OBJ_MAX_NAME] = '\0';
			Global::getFileDirectory(filename, OBJ_MAX_PATH, m_path);
			strcat(filename, libName);

			FILE * libFile = fopen(filename, "rb");
			if( !libFile ) continue;
			importMaterialAllocate(libFile);
			importMaterialData(libFile);
			fclose(libFile);
		}
		len = readLine(file);
	}
}
//-----------------------------------------------------------------------------
void ObjFile::importMaterialAllocate(FILE* file)
{
	int lastNoMaterials = m_noMaterials;

	int start = ftell(file);
	int len = readLine(file);
	while( len )
	{
		if( strncmp(m_line, "newmtl ", 7) == 0 ) m_noMaterials++;
		len = readLine(file);
	}

	fseek(file, start, SEEK_SET);
	if( m_noMaterials != lastNoMaterials )
	{
		ObjMaterial * newMaterials = new ObjMaterial[m_noMaterials];
		for( int m = 0; m < lastNoMaterials; m++ )
			newMaterials[m] = m_materials[m];
		if( m_materials ) delete[] m_materials;
		m_materials = newMaterials;
	}
}
//-----------------------------------------------------------------------------
void ObjFile::importMaterialData(FILE* file)
{
	int materialIndex = -1;
	int start = ftell(file);
	int len = readLine(file);
	while( len )
	{
		if( strncmp(m_line, "newmtl ", 7) == 0 )
		{
			materialIndex++;
			strncpy(m_materials[materialIndex].name, &m_line[7], OBJ_MAX_NAME);
			m_materials[materialIndex].name[OBJ_MAX_NAME] = '\0';
		}
		else
		{
			if( materialIndex < 0 ) continue;
			if( strncmp(m_line, "Ka ", 3) == 0 )
			{
				readColor(&m_line[3], m_materials[materialIndex].ambient);
			}
			else if( strncmp(m_line, "Kd ", 3) == 0 )
			{
				readColor(&m_line[3], m_materials[materialIndex].diffuse);
			}
			else if( strncmp(m_line, "Ks ", 3) == 0 )
			{
				readColor(&m_line[3], m_materials[materialIndex].specular);
			}
			else if( strncmp(m_line, "Ns ", 3) == 0 )
			{
				sscanf(&m_line[3], "%f", &m_materials[materialIndex].shininess);
			}
			else if( strncmp(m_line, "d ", 2) == 0 )
			{
				sscanf(&m_line[2], "%f", &m_materials[materialIndex].transparency);
			}
			else if( strncmp(m_line, "map_Kd ", 7) == 0 )
			{
				strncpy(m_materials[materialIndex].texture, &m_line[7], OBJ_MAX_NAME);
				m_materials[materialIndex].texture[OBJ_MAX_NAME] = '\0';
			}
		}
		len = readLine(file);
	}

	fseek(file, start, SEEK_SET);
}
//-----------------------------------------------------------------------------
ObjMaterial* ObjFile::getMaterialFromName(const char* name)
{
	for( int i = 0; i < m_noMaterials; i++ )
		if( strcmp(name, m_materials[i].name) == 0 )
			return &m_materials[i];
	return &defMaterial;
}
//-----------------------------------------------------------------------------
void ObjFile::importMeshAllocate(FILE* file, Mesh* mesh)
{
	int start = ftell(file);
	int len = readLine(file);
	while( len )
	{
		if( strncmp(m_line, "v ", 2) == 0 ) mesh->noVertexes++;
		else if( strncmp(m_line, "vt ", 3) == 0 ) mesh->noTexCoords++;
		else if( strncmp(m_line, "vn ", 3) == 0 ) m_noNormals++;
		else if( strncmp(m_line, "f ", 2) == 0 ) mesh->noTriangles++;
		else if( strncmp(m_line, "o ", 2) == 0 ) break;
		len = readLine(file);
	}

	fseek(file, start, SEEK_SET);
	mesh->Allocate(mesh->noVertexes, mesh->noTexCoords, mesh->noTriangles);
	if( m_noNormals ) mesh->AllocateNormals();
	m_normals = new Vertex[m_noNormals];
}
//-----------------------------------------------------------------------------
void ObjFile::importMeshData(FILE* file, Mesh* mesh)
{
	int vertexesIndex = 0;
	int texCoordsIndex = 0;
	int normalsIndex = 0;
	int trianglesIndex = 0;

	m_curMaterial = &defMaterial;

	int start = ftell(file);
	int len = readLine(file);
	while( len )
	{
		if( strncmp(m_line, "v ", 2) == 0 ) readVertex(mesh, vertexesIndex++);
		else if( strncmp(m_line, "vt ", 3) == 0 ) readTexCoord(mesh, texCoordsIndex++);
		else if( strncmp(m_line, "vn ", 3) == 0 ) readNormal(mesh, normalsIndex++);
		else if( strncmp(m_line, "f ", 2) == 0 )
		{
			readTriangle(mesh, trianglesIndex);
			mesh->colors[trianglesIndex] = m_curMaterial->diffuse;
			int slot = 0;
			if( m_curMaterial->texture[0] )
			{
				slot = gImageCache.GetSlotFromName(m_curMaterial->texture);
				if( !slot ) printf("objFile: using default texture (instead of %s)!\n", m_curMaterial->texture);
			}
			mesh->texSlotList[trianglesIndex++] = slot;
		}
		else if( strncmp(m_line, "usemtl ", 7) == 0 )
		{
			m_curMaterial = getMaterialFromName(&m_line[7]);
		}
		else if( strncmp(m_line, "o ", 2) == 0 ) break;
		len = readLine(file);
	}

	fseek(file, start, SEEK_SET);
	if( m_normals )
	{
		delete[] m_normals;
		m_noNormals = 0;
	}
}
//-----------------------------------------------------------------------------
void ObjFile::readVertex(Mesh* mesh, int index)
{
	Vertex v;
	sscanf(m_line, "v %f %f %f %f", &v.x, &v.y, &v.z, &v.w);
	mesh->vertexes[index] = v;
}
//-----------------------------------------------------------------------------
void ObjFile::readTexCoord(Mesh* mesh, int index)
{
	float u = 0.0f, v = 0.0f, w = 0.0f;
	sscanf(m_line, "vt %f %f %f", &u, &v, &w);
	mesh->texCoords[index * 2] = u;
	mesh->texCoords[index * 2 + 1] = 1.0f - v;
}
//-----------------------------------------------------------------------------
void ObjFile::readNormal(Mesh* mesh, int index)
{
	Vertex n;
	sscanf(m_line, "vn %f %f %f", &n.x, &n.y, &n.z);
	m_normals[index] = n;
}
//-----------------------------------------------------------------------------
void ObjFile::readTriangle(Mesh* mesh, int index)
{
	int* vList = &mesh->vertexesList[index * 3];
	int* tList = &mesh->texCoordsList[index * 3];

	const char* buffer = &m_line[2];
	for( int i = 0; i < 3; i++ )
	{
		int v = 0, t = 0, n = 0;
		bool vOk = true, tOk = true, nOk = true;
		int r = sscanf(buffer, "%i/%i/%i", &v, &t, &n);
		if( r != 3 )
		{
			vOk = true, tOk = true, nOk = false;
			r = sscanf(buffer, "%i/%i", &v, &t);
			if( r != 2 )
			{
				vOk = true, tOk = false, nOk = true;
				r = sscanf(buffer, "%i//%i", &v, &n);
				if( r != 2 )
				{
					vOk = true, tOk = false, nOk = false;
					r = sscanf(buffer, "%i", &v);
					if( r != 1 )
					{
						vOk = false, tOk = false, nOk = false;
					}
				}
			}
		}

		if( vOk )
		{
			if( v <= 0 || v > mesh->noVertexes )
			{
				printf("objFile: error detected in vertex indices!\n");
				continue;
			}
			vList[i] = v - 1;
		}

		if( tOk )
		{
			if( t <= 0 || t > mesh->noTexCoords )
			{
				printf("objFile: error detected in texture coordinate indices!\n");
				continue;
			}
			tList[i] = t - 1;
		}

		if( nOk )
		{
			if( n <= 0 || n > m_noNormals )
			{
				printf("objFile: error detected in normal indices!\n");
				continue;
			}
			mesh->normals[index] = m_normals[n - 1];
		}

		buffer = strchr(buffer, ' ');
		if( !buffer ) break;
		buffer++;
	}
}
//-----------------------------------------------------------------------------
void ObjFile::readColor(const char* buffer, Color& color)
{
	float r = 0.0f, g = 0.0f, b = 0.0f;
	sscanf(buffer, "%f %f %f", &r, &g, &b);
	color = Color(
		(uint8_t)cmbound(r * 255.0f, 0.0f, 255.0f),
		(uint8_t)cmbound(g * 255.0f, 0.0f, 255.0f),
		(uint8_t)cmbound(b * 255.0f, 0.0f, 255.0f),
		0
	);
}
//-----------------------------------------------------------------------------
int ObjFile::readLine(FILE* file)
{
	int len = 0;
	bool skip = false;

	while( !feof(file) )
	{
		int c = fgetc(file);
		if( c == '\n' || c == '\r' )
		{
			if( len != 0 ) break;
			else
			{
				skip = false;
				continue;
			}
		}

		if( skip || c == '#' )
		{
			skip = true;
			continue;
		}

		if( len < OBJ_MAX_LINE )
			m_line[len++] = c;
	}

	m_line[len] = '\0';
	return len;
}
//-----------------------------------------------------------------------------
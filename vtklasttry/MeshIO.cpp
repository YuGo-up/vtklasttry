#include "MeshIO.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>

#include <vtkCellArray.h>
#include <vtkDataSet.h>
#include <vtkDataSetReader.h>
#include <vtkIdList.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkSTLReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridWriter.h>

namespace
{
static QString cacheDirPath()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty())
        base = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString dir = QDir(base).filePath(QStringLiteral("cache"));
    QDir().mkpath(dir);
    return dir;
}

static QString makeCacheKeyForFile(const QString& filePath)
{
    const QFileInfo fi(filePath);
    const QString sig = QString("%1|%2|%3|%4")
                            .arg(fi.absoluteFilePath())
                            .arg(fi.size())
                            .arg(fi.lastModified().toMSecsSinceEpoch())
                            .arg(fi.fileName());
    const QByteArray h = QCryptographicHash::hash(sig.toUtf8(), QCryptographicHash::Sha1).toHex();
    return QString::fromLatin1(h);
}

static QString cachedVtuPathForSource(const QString& sourceReadablePath)
{
    const QString key = makeCacheKeyForFile(sourceReadablePath);
    return QDir(cacheDirPath()).filePath(QString("legacy51_%1.vtu").arg(key));
}

static bool fileLooksLikeVtk51OffsetsConnectivity(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    const QByteArray head = f.read(256 * 1024);
    return head.contains("vtk DataFile Version 5.") && head.contains("DATASET UNSTRUCTURED_GRID") &&
           head.contains("CELLS") && (head.contains("OFFSETS") && head.contains("CONNECTIVITY"));
}

static vtkSmartPointer<vtkUnstructuredGrid> readLegacyVtk51UnstructuredGridAscii(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return nullptr;

    QTextStream in(&f);
    in.setCodec("UTF-8");

    auto nextToken = [&]() -> QString {
        QString tok;
        while (!in.atEnd())
        {
            in >> tok;
            if (!tok.isEmpty())
                return tok;
        }
        return QString();
    };

    int pointCount = 0;
    int cellCount = 0;

    vtkNew<vtkPoints> points;
    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    QVector<vtkIdType> offsets;
    QVector<vtkIdType> connectivity;
    QVector<int> cellTypes;

    QString tok;
    while (!(tok = nextToken()).isEmpty() && tok != "POINTS")
    {
    }
    if (tok != "POINTS")
        return nullptr;

    pointCount = nextToken().toInt();
    (void)nextToken(); // scalar type
    points->SetNumberOfPoints(pointCount);
    for (int i = 0; i < pointCount; ++i)
    {
        const double x = nextToken().toDouble();
        const double y = nextToken().toDouble();
        const double z = nextToken().toDouble();
        points->SetPoint(i, x, y, z);
    }
    grid->SetPoints(points);

    while (!(tok = nextToken()).isEmpty() && tok != "CELLS")
    {
    }
    if (tok != "CELLS")
        return nullptr;

    cellCount = nextToken().toInt();
    (void)nextToken(); // total size

    tok = nextToken();
    if (tok != "OFFSETS")
        return nullptr;
    (void)nextToken(); // offsets type
    offsets.resize(cellCount + 1);
    for (int i = 0; i < cellCount + 1; ++i)
        offsets[i] = static_cast<vtkIdType>(nextToken().toLongLong());

    tok = nextToken();
    if (tok != "CONNECTIVITY")
        return nullptr;
    (void)nextToken(); // connectivity type
    const vtkIdType connLen = offsets.isEmpty() ? 0 : offsets.last();
    if (connLen <= 0)
        return nullptr;
    connectivity.resize(static_cast<int>(connLen));
    for (vtkIdType i = 0; i < connLen; ++i)
        connectivity[static_cast<int>(i)] = static_cast<vtkIdType>(nextToken().toLongLong());

    vtkNew<vtkIdList> ids;
    for (int ci = 0; ci < cellCount; ++ci)
    {
        const vtkIdType start = offsets[ci];
        const vtkIdType end = offsets[ci + 1];
        const vtkIdType n = end - start;
        if (n <= 0)
            continue;
        ids->SetNumberOfIds(n);
        for (vtkIdType k = 0; k < n; ++k)
            ids->SetId(k, connectivity[static_cast<int>(start + k)]);
        cells->InsertNextCell(ids);
    }

    while (!(tok = nextToken()).isEmpty() && tok != "CELL_TYPES")
    {
    }
    if (tok == "CELL_TYPES")
    {
        const int nTypes = nextToken().toInt();
        cellTypes.resize(nTypes);
        for (int i = 0; i < nTypes; ++i)
            cellTypes[i] = nextToken().toInt();
    }

    const int nCellsBuilt = static_cast<int>(cells->GetNumberOfCells());
    if (nCellsBuilt <= 0)
        return nullptr;

    if (cellTypes.size() < nCellsBuilt && !cellTypes.isEmpty())
    {
        const int last = cellTypes.last();
        while (cellTypes.size() < nCellsBuilt)
            cellTypes.push_back(last);
    }
    else if (cellTypes.size() < nCellsBuilt && cellTypes.isEmpty())
    {
        cellTypes.resize(nCellsBuilt);
        for (int i = 0; i < nCellsBuilt; ++i)
            cellTypes[i] = 3; // VTK_LINE
    }

    grid->SetCells(cellTypes.data(), cells);
    if (grid->GetNumberOfPoints() == 0 || grid->GetNumberOfCells() == 0)
        return nullptr;
    return grid;
}

static QString ensureReadableAsciiPathIfNeeded(const QString& path, const QString& ext, const vtklasttry::LogFn& log,
                                              QString* outMsg)
{
    QString readablePath = path;
    bool hasNonAscii = false;
    for (QChar ch : readablePath)
    {
        if (ch.unicode() > 127)
        {
            hasNonAscii = true;
            break;
        }
    }
    if (!hasNonAscii)
        return readablePath;

    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir().mkpath(tempDir);
    const QString tempPath = QDir(tempDir).filePath(QStringLiteral("vtklasttry_mesh.%1").arg(ext));
    if (QFile::exists(tempPath))
        QFile::remove(tempPath);
    if (QFile::copy(readablePath, tempPath))
    {
        readablePath = tempPath;
        if (log)
            log(QString::fromWCharArray(L"\u68C0\u6D4B\u5230\u975E ASCII \u8DEF\u5F84\uFF0C\u5DF2\u590D\u5236\u5230\u4E34\u65F6\u8DEF\u5F84\u8BFB\u53D6\uFF1A%1").arg(tempPath));
    }
    else
    {
        if (outMsg)
            *outMsg = QString::fromWCharArray(L"\u975E ASCII \u8DEF\u5F84\u590D\u5236\u5931\u8D25\uFF0C\u5C06\u76F4\u63A5\u5C1D\u8BD5\u539F\u8DEF\u5F84\u3002");
    }
    return readablePath;
}
} // namespace

namespace vtklasttry
{
MeshLoadResult loadMesh(const QString& path, const LogFn& log)
{
    MeshLoadResult r;
    r.sourcePath = path;

    const QString ext = QFileInfo(path).suffix().toLower();
    r.readablePath = ensureReadableAsciiPathIfNeeded(path, ext, log, &r.message);

    vtkSmartPointer<vtkDataSet> ds;

    if (ext == "stl")
    {
        if (log) log(QString::fromWCharArray(L"\u6309 STL \u683C\u5F0F\u8BFB\u53D6\u2026"));
        r.pipeline = QStringLiteral("STL -> vtkSTLReader");
        vtkNew<vtkSTLReader> reader;
        reader->SetFileName(r.readablePath.toLocal8Bit().constData());
        reader->Update();
        ds = reader->GetOutput();
    }
    else if (ext == "vtk")
    {
        if (log) log(QString::fromWCharArray(L"\u6309 VTK legacy \u683C\u5F0F\u8BFB\u53D6\u2026"));
        r.pipeline = QStringLiteral("VTK legacy -> (legacy5.1 cache) or vtkUnstructuredGridReader/vtkDataSetReader");
        if (fileLooksLikeVtk51OffsetsConnectivity(r.readablePath))
        {
            const QString cacheVtu = cachedVtuPathForSource(r.readablePath);
            if (QFile::exists(cacheVtu))
            {
                if (log) log(QString::fromWCharArray(L"\u68C0\u6D4B\u5230 legacy 5.1\uFF0C\u4F7F\u7528\u7F13\u5B58 VTU\uFF1A%1").arg(cacheVtu));
                r.pipeline = QString("VTK legacy 5.1 -> cached VTU (%1)").arg(cacheVtu);
                vtkNew<vtkXMLUnstructuredGridReader> rr;
                rr->SetFileName(cacheVtu.toLocal8Bit().constData());
                rr->Update();
                if (rr->GetOutput() && rr->GetOutput()->GetNumberOfCells() > 0)
                {
                    ds = rr->GetOutput();
                    r.ok = true;
                    r.data = ds;
                    return r;
                }
                if (log) log(QString::fromWCharArray(L"\u7F13\u5B58 VTU \u8BFB\u53D6\u5931\u8D25\uFF0C\u5C06\u91CD\u65B0\u8F6C\u6362\u3002"));
            }

            if (log) log(QString::fromWCharArray(L"\u68C0\u6D4B\u5230 VTK 5.x OFFSETS/CONNECTIVITY\uFF0C\u5C1D\u8BD5\u81EA\u52A8\u8F6C\u6362\u4E3A .vtu \u5E76\u7F13\u5B58\u2026"));
            r.pipeline = QString("VTK legacy 5.1 -> parse -> write cached VTU (%1)").arg(cacheVtu);
            QElapsedTimer t;
            t.start();
            vtkSmartPointer<vtkUnstructuredGrid> g = readLegacyVtk51UnstructuredGridAscii(r.readablePath);
            if (g && g->GetNumberOfCells() > 0)
            {
                vtkNew<vtkXMLUnstructuredGridWriter> w;
                w->SetFileName(cacheVtu.toLocal8Bit().constData());
                w->SetInputData(g);
                w->SetDataModeToBinary();
                const int ok = w->Write();
                if (ok)
                {
                    if (log) log(QString::fromWCharArray(L"\u8F6C\u6362\u6210\u529F\uFF0C\u5DF2\u7F13\u5B58\uFF1A%1\uFF08%2 ms\uFF09").arg(cacheVtu).arg(t.elapsed()));
                }
                else
                {
                    if (log) log(QString::fromWCharArray(L"\u8F6C\u6362\u6210\u529F\uFF0C\u4F46\u5199\u5165\u7F13\u5B58\u5931\u8D25\uFF0C\u5C06\u76F4\u63A5\u4F7F\u7528\u5F53\u524D\u89E3\u6790\u7ED3\u679C\u3002"));
                }
                ds = g;
                r.ok = true;
                r.data = ds;
                return r;
            }
            r.ok = false;
            r.message = QString::fromWCharArray(L"\u68C0\u6D4B\u5230 VTK 5.1 OFFSETS/CONNECTIVITY\uFF0C\u4F46\u81EA\u52A8\u8F6C\u6362\u5931\u8D25\u3002");
            return r;
        }

        vtkNew<vtkUnstructuredGridReader> ugr;
        ugr->SetFileName(r.readablePath.toLocal8Bit().constData());
        ugr->Update();
        if (ugr->GetOutput() && ugr->GetOutput()->GetNumberOfCells() > 0)
        {
            ds = ugr->GetOutput();
        }
        else
        {
            vtkNew<vtkDataSetReader> reader;
            reader->SetFileName(r.readablePath.toLocal8Bit().constData());
            reader->Update();
            ds = reader->GetOutput();
        }
    }
    else if (ext == "vtu")
    {
        if (log) log(QString::fromWCharArray(L"\u6309 VTU \u683C\u5F0F\u8BFB\u53D6\u2026"));
        r.pipeline = QStringLiteral("VTU -> vtkXMLUnstructuredGridReader");
        vtkNew<vtkXMLUnstructuredGridReader> reader;
        reader->SetFileName(r.readablePath.toLocal8Bit().constData());
        reader->Update();
        ds = reader->GetOutput();
    }
    else if (ext == "vtp")
    {
        if (log) log(QString::fromWCharArray(L"\u6309 VTP \u683C\u5F0F\u8BFB\u53D6\u2026"));
        r.pipeline = QStringLiteral("VTP -> vtkXMLPolyDataReader");
        vtkNew<vtkXMLPolyDataReader> reader;
        reader->SetFileName(r.readablePath.toLocal8Bit().constData());
        reader->Update();
        ds = reader->GetOutput();
    }
    else if (ext == "inp")
    {
        if (log) log(QString::fromWCharArray(L"\u6309 Abaqus INP \u683C\u5F0F\u8BFB\u53D6\u2026"));
        r.pipeline = QStringLiteral("INP -> custom parser (CPS4)");

        QFile f(r.readablePath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            r.ok = false;
            r.message = QString::fromWCharArray(L"\u65E0\u6CD5\u6253\u5F00 INP \u6587\u4EF6\u3002");
            return r;
        }
        QTextStream in(&f);
        in.setCodec("UTF-8");

        QHash<int, vtkIdType> nodeMap;
        nodeMap.reserve(10000);
        vtkNew<vtkPoints> points;
        points->SetDataTypeToDouble();

        vtkSmartPointer<vtkUnstructuredGrid> ug = vtkSmartPointer<vtkUnstructuredGrid>::New();
        ug->SetPoints(points);

        bool inNode = false;
        bool inElem = false;
        QString elemType;

        auto trim = [](QString s) { return s.trimmed(); };
        auto parseCsvInts = [&](const QString& line) -> QVector<int> {
            QVector<int> vals;
            const QStringList parts = line.split(',', Qt::SkipEmptyParts);
            vals.reserve(parts.size());
            for (const QString& p : parts)
                vals.push_back(trim(p).toInt());
            return vals;
        };
        auto parseCsvDoubles = [&](const QString& line) -> QVector<double> {
            QVector<double> vals;
            const QStringList parts = line.split(',', Qt::SkipEmptyParts);
            vals.reserve(parts.size());
            for (const QString& p : parts)
                vals.push_back(trim(p).toDouble());
            return vals;
        };

        vtkNew<vtkIdList> ids;
        while (!in.atEnd())
        {
            QString line = in.readLine();
            const QString l = line.trimmed();
            if (l.isEmpty() || l.startsWith("**"))
                continue;
            if (l.startsWith("*"))
            {
                inNode = false;
                inElem = false;
                const QString upper = l.toUpper();
                if (upper.startsWith("*NODE"))
                {
                    inNode = true;
                    continue;
                }
                if (upper.startsWith("*ELEMENT"))
                {
                    inElem = true;
                    elemType.clear();
                    const int idx = upper.indexOf("TYPE=");
                    if (idx >= 0)
                    {
                        elemType = upper.mid(idx + 5).trimmed();
                        const int comma = elemType.indexOf(',');
                        if (comma >= 0)
                            elemType = elemType.left(comma).trimmed();
                    }
                    continue;
                }
                continue;
            }
            if (inNode)
            {
                const QVector<double> vals = parseCsvDoubles(line);
                if (vals.size() < 4)
                    continue;
                const int nid = static_cast<int>(vals[0]);
                const vtkIdType pid = points->InsertNextPoint(vals[1], vals[2], vals[3]);
                nodeMap.insert(nid, pid);
            }
            else if (inElem)
            {
                const QVector<int> vals = parseCsvInts(line);
                if (vals.size() < 2)
                    continue;
                QVector<vtkIdType> pids;
                for (int i = 1; i < vals.size(); ++i)
                {
                    if (!nodeMap.contains(vals[i]))
                        continue;
                    pids.push_back(nodeMap.value(vals[i]));
                }
                if (elemType == "CPS4" && pids.size() == 4)
                {
                    ids->SetNumberOfIds(4);
                    ids->SetId(0, pids[0]);
                    ids->SetId(1, pids[1]);
                    ids->SetId(2, pids[2]);
                    ids->SetId(3, pids[3]);
                    ug->InsertNextCell(VTK_QUAD, ids);
                }
            }
        }

        if (ug->GetNumberOfPoints() <= 0 || ug->GetNumberOfCells() <= 0)
        {
            r.ok = false;
            r.message = QString::fromWCharArray(L"INP \u89E3\u6790\u540E\u672A\u5F97\u5230\u6709\u6548\u8282\u70B9/\u5355\u5143\uFF08\u5F53\u524D\u4EC5\u5B9E\u73B0 CPS4\uFF09\u3002");
            return r;
        }

        if (log) log(QString("INP parsed: points=%1, cells=%2, elemType=%3").arg(ug->GetNumberOfPoints()).arg(ug->GetNumberOfCells()).arg(elemType));
        ds = ug;
    }

    r.data = ds;
    r.ok = (ds && ds->GetNumberOfCells() > 0);
    return r;
}
} // namespace vtklasttry


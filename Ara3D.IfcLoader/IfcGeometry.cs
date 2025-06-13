using Ara3D.Logging;
using System.Runtime.InteropServices;

namespace Ara3D.IfcLoader
{
    public class IfcGeometry
    {
        public readonly IntPtr ApiPtr;
        public readonly IntPtr GeometryPtr;
        public readonly int NumMeshes;
        public readonly uint Id;
        public readonly string Guid;
        public readonly string EntityType;
        public readonly uint EntityLabel;
        public readonly uint EntityTypeId;

        public IfcGeometry(IntPtr apiPtr, IntPtr geometryPtr, IntPtr modelPtr)
        {
            ApiPtr = apiPtr;
            GeometryPtr = geometryPtr;
            Id = WebIfcDll.GetMeshId(ApiPtr, GeometryPtr);
            NumMeshes = WebIfcDll.GetNumMeshes(ApiPtr, GeometryPtr);
            Guid = ExtractGuid(geometryPtr, modelPtr);
            EntityType = ExtractEntityType(geometryPtr, modelPtr);
            EntityTypeId = ExtractEntityTypeId(geometryPtr, modelPtr);
            EntityLabel = ExtractEntityLabel(geometryPtr, modelPtr);
        }

        private static string ExtractGuid(IntPtr geometryPtr, IntPtr modelPtr)
        {
            if (geometryPtr == IntPtr.Zero)
            {
                return string.Empty;
            }

            var guidPtr = WebIfcDll.GetGuid(modelPtr, geometryPtr);
            var guid = Marshal.PtrToStringAnsi(guidPtr) ?? string.Empty;
            WebIfcDll.FreeString(guidPtr);

            return guid;
        }

        private static string ExtractEntityType(IntPtr geometryPtr, IntPtr modelPtr)
        {
            if (geometryPtr == IntPtr.Zero)
            {
                return string.Empty;
            }

            var entityTypePtr = WebIfcDll.GetEntityType(modelPtr, geometryPtr);
            var entityType = Marshal.PtrToStringAnsi(entityTypePtr) ?? string.Empty;
            WebIfcDll.FreeString(entityTypePtr);

            return entityType;
        }

        private static uint ExtractEntityTypeId(IntPtr geometryPtr, IntPtr modelPtr)
        {
            if (geometryPtr == IntPtr.Zero)
            {
                return 0;
            }

            return WebIfcDll.GetEntityTypeId(modelPtr, geometryPtr);
        }

        private static uint ExtractEntityLabel(IntPtr geometryPtr, IntPtr modelPtr)
        {
            if (geometryPtr == IntPtr.Zero)
            {
                return 0;
            }
            
            return WebIfcDll.GetEntityLabel(modelPtr, geometryPtr);

            //var id = WebIfcDll.GetEntityLabel(modelPtr, geometryPtr);

            //var entityTypePtr = WebIfcDll.GetEntityType(modelPtr, geometryPtr);
            //var entityType = Marshal.PtrToStringAnsi(entityTypePtr) ?? string.Empty;

            //if (entityType == "IfcFooting")
            //{
            //    var stop = true;
            //}

            //if (id == 23286)
            //{
            //    var pause = true;
            //}

            //if (id == 23369)
            //{
            //    var pause = true;
            //}

            //if (id == 23408)
            //{
            //    var pause = true;
            //}
            //return id;
        }

        public IfcMesh GetMesh(int i) 
            => new IfcMesh(ApiPtr, WebIfcDll.GetMesh(ApiPtr, GeometryPtr, i));

        public int GetNumMeshes()
            => NumMeshes;

        public IEnumerable<IfcMesh> GetMeshes()
        {
            for (int i = 0; i < NumMeshes; ++i)
                yield return GetMesh(i);
        }
    }
}
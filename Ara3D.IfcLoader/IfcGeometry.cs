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

        public IfcGeometry(IntPtr apiPtr, IntPtr geometryPtr, IntPtr modelPtr)
        {
            ApiPtr = apiPtr;
            GeometryPtr = geometryPtr;
            Id = WebIfcDll.GetMeshId(ApiPtr, GeometryPtr);
            NumMeshes = WebIfcDll.GetNumMeshes(ApiPtr, GeometryPtr);
            Guid = ExtractGuid(geometryPtr, modelPtr);
            EntityType = ExtractEntityType(geometryPtr, modelPtr);
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
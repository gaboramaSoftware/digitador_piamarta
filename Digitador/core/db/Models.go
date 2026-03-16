package db

type RolUsuario int

const (
	RolInvalido      RolUsuario = 0
	RolAdministrador RolUsuario = 1
	RolOperador      RolUsuario = 2
	RolEstudiante    RolUsuario = 3
)

type TipoRacion int

const (
	RacionNoAplica TipoRacion = 0
	Desayuno       TipoRacion = 1
	Almuerzo       TipoRacion = 2
)

type EstadoSemaforo int

const (
	AprobadoPAE       EstadoSemaforo = 1
	AprobadoNormal    EstadoSemaforo = 2
	RechazadoDoble    EstadoSemaforo = 3
	RechazadoNoPAE    EstadoSemaforo = 4
	RechazadoNoExiste EstadoSemaforo = 5
)

type EstadoRegistro int

const (
	Pendiente    EstadoRegistro = 0
	Sincronizado EstadoRegistro = 1
)

// -- Estructuras de tablas de dominio --
type Usuario struct {
	RunID          string     `json:"run_id"`
	DV             string     `json:"dv"` // En Go es mejor manejar el char como string corto
	NombreCompleto string     `json:"nombre_completo"`
	IDRol          RolUsuario `json:"id_rol"`
	PasswordHash   string     `json:"-"` // El guion indica que nunca se envíe por WebSocket (Seguridad)
	TemplateHuella []byte     `json:"-"` // []byte es el equivalente exacto a std::vector<uint8_t>
}

type RegistroRacion struct {
	IDRegistro     int64          `json:"id_registro"`
	IDEstudiante   string         `json:"id_estudiante"`
	FechaServicio  string         `json:"fecha_servicio"`
	TipoRacion     TipoRacion     `json:"tipo_racion"`
	IDTerminal     string         `json:"id_terminal"`
	HoraEvento     int64          `json:"hora_evento"`
	EstadoRegistro EstadoRegistro `json:"estado_registro"`
}

type DetalleEstudiante struct {
	RunID string `json:"run_id"`
	Curso string `json:"curso"`
	Letra string `json:"letra"`
}

type Configuracion struct {
	IDTerminal      string     `json:"id_terminal"`
	TipoRacion      TipoRacion `json:"tipo_racion"`
	PuertoImpresora string     `json:"puerto_impresora"`
}

// -- Data Transfer Objects (DTO) --

type PerfilEstudiante struct {
	RunID          string     `json:"run_id"`
	DV             string     `json:"dv"`
	NombreCompleto string     `json:"nombre_completo"`
	IDRol          RolUsuario `json:"id_rol"`
	TemplateHuella []byte     `json:"-"`
	Curso          string     `json:"curso"`
	Letra          string     `json:"letra"`
}

type RequestEnrolarUsuario struct {
	RunNuevo       string `json:"run_nuevo"`
	DVNuevo        string `json:"dv_nuevo"`
	NombreNuevo    string `json:"nombre_nuevo"`
	CursoNuevo     string `json:"curso_nuevo"`
	LetraNueva     string `json:"letra_nueva"`
	TemplateHuella []byte `json:"-"`
}

// dto para hacer un join entre las tablas
// asi ajilizar el dashboard
type RegistroRecienteDTO struct {
	ID             int64          `json:"id_registro"`
	NombreCompleto string         `json:"nombre_completo"`
	Run            string         `json:"run"`
	Curso          string         `json:"curso"`
	Letra          string         `json:"letra"`
	FechaServicio  string         `json:"fecha_servicio"`
	HoraEvento     int64          `json:"hora_evento"`
	TipoRacion     TipoRacion     `json:"tipo_racion"`
	NUC            string         `json:"id_terminal"`
	EstadoRegistro EstadoRegistro `json:"estado_registro"`
}

type HistorialRacion struct {
	Fecha  string `json:"fecha"`
	Hora   string `json:"hora"`
	Tipo   string `json:"tipo"`
	Estado string `json:"estado"`
}

type Stats struct {
	Desayunos int `json:"desayunos"`
	Almuerzos int `json:"almuerzos"`
	Total     int `json:"total"`
}

// -- mapping y adaptadores--
type DB interface {
	ObtenerTodosTemplates() (map[string][]byte, error)
	GetUser(runID string) (*Usuario, error)
	ObtenerPerfilPorRunID(runID string) (*PerfilEstudiante, error)
}

// match_treshold es el umbral de coincidencia para comparar huellas
const MatchTreshold = 300

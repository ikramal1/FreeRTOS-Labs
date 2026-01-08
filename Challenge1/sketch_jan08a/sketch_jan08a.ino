// ============================================================
// CONFIGURATION MULTI-CORE ESP32
// ============================================================

// L'ESP32 a 2 cœurs: Core 0 (WiFi/BT) et Core 1 (application)
// On utilise Core 1 pour nos tâches pour éviter les interférences
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;  // Mode 1 cœur
#else
  static const BaseType_t app_cpu = 1;  // Mode 2 cœurs (standard)
#endif

// ============================================================
// CONSTANTES ET CONFIGURATION
// ============================================================

// Taux de clignotement en millisecondes
static const int RATE_FAST = 300;   // Clignotement rapide (300ms)
static const int RATE_SLOW = 1000;  // Clignotement lent (1000ms)

// Configuration matérielle
static const int LED_PIN = 2;  // GPIO2 sur la plupart des ESP32

// Configuration FreeRTOS
static const uint16_t STACK_SIZE = 2048;  // Taille de pile par tâche (en bytes)
static const UBaseType_t TASK_PRIORITY = 1;  // Priorité des tâches (1 = normale)

// Timeout pour l'acquisition du mutex
static const TickType_t MUTEX_TIMEOUT = pdMS_TO_TICKS(200);  // 200ms max d'attente

// ============================================================
// VARIABLES GLOBALES
// ============================================================

// Handle du mutex pour synchroniser l'accès à la LED
SemaphoreHandle_t ledMutex = NULL;

// Compteurs pour le monitoring (optionnel, pour débogage)
volatile uint32_t fastBlinkCount = 0;
volatile uint32_t slowBlinkCount = 0;

// ============================================================
// TÂCHE 1: CLIGNOTEMENT RAPIDE
// ============================================================

void taskFastBlink(void *parameter) {
  // Message de démarrage
  Serial.println("[TASK FAST] Démarrée sur Core " + String(xPortGetCoreID()));
  
  // Boucle infinie de la tâche
  while (true) {
    
    // Tentative d'acquisition du mutex avec timeout
    if (xSemaphoreTake(ledMutex, MUTEX_TIMEOUT) == pdTRUE) {
      
      // === SECTION CRITIQUE: Accès exclusif à la LED ===
      
      // Allumer la LED
      digitalWrite(LED_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(RATE_FAST));
      
      // Éteindre la LED
      digitalWrite(LED_PIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(RATE_FAST));
      
      // Incrémenter le compteur (pour stats)
      fastBlinkCount++;
      
      // === FIN SECTION CRITIQUE ===
      
      // Libérer le mutex pour les autres tâches
      xSemaphoreGive(ledMutex);
      
    } else {
      // Timeout: le mutex n'a pas pu être acquis
      Serial.println("[TASK FAST] Timeout mutex!");
    }
    
    // Petite pause avant la prochaine tentative
    // Évite la monopolisation du CPU
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  
  // Cette ligne n'est jamais atteinte (tâche infinie)
  // Mais en production, on devrait supprimer la tâche proprement:
  // vTaskDelete(NULL);
}

// ============================================================
// TÂCHE 2: CLIGNOTEMENT LENT
// ============================================================

void taskSlowBlink(void *parameter) {
  // Message de démarrage
  Serial.println("[TASK SLOW] Démarrée sur Core " + String(xPortGetCoreID()));
  
  // Boucle infinie de la tâche
  while (true) {
    
    // Tentative d'acquisition du mutex avec timeout
    if (xSemaphoreTake(ledMutex, MUTEX_TIMEOUT) == pdTRUE) {
      
      // === SECTION CRITIQUE: Accès exclusif à la LED ===
      
      // Allumer la LED
      digitalWrite(LED_PIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(RATE_SLOW));
      
      // Éteindre la LED
      digitalWrite(LED_PIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(RATE_SLOW));
      
      // Incrémenter le compteur (pour stats)
      slowBlinkCount++;
      
      // === FIN SECTION CRITIQUE ===
      
      // Libérer le mutex pour les autres tâches
      xSemaphoreGive(ledMutex);
      
    } else {
      // Timeout: le mutex n'a pas pu être acquis
      Serial.println("[TASK SLOW] Timeout mutex!");
    }
    
    // Petite pause avant la prochaine tentative
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  
  // Cette ligne n'est jamais atteinte (tâche infinie)
  // vTaskDelete(NULL);
}

// ============================================================
// TÂCHE 3: MONITORING (OPTIONNEL)
// ============================================================

void taskMonitor(void *parameter) {
  Serial.println("[TASK MONITOR] Démarrée sur Core " + String(xPortGetCoreID()));
  
  while (true) {
    // Afficher les statistiques toutes les 10 secondes
    vTaskDelay(pdMS_TO_TICKS(10000));
    
    Serial.println("\n=== STATISTIQUES ===");
    Serial.println("Clignotements rapides: " + String(fastBlinkCount));
    Serial.println("Clignotements lents: " + String(slowBlinkCount));
    Serial.println("Mémoire libre: " + String(esp_get_free_heap_size()) + " bytes");
    Serial.println("====================\n");
  }
}

// ============================================================
// FONCTION SETUP: INITIALISATION
// ============================================================

void setup() {
  
  // Initialisation du port série
  Serial.begin(115200);
  
  // Attendre que le port série soit prêt
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║  ESP32 FreeRTOS - Code Optimal v2.0   ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  
  // Configuration de la LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // LED éteinte au départ
  Serial.println("[SETUP] LED configurée sur GPIO " + String(LED_PIN));
  
  // ============================================================
  // ÉTAPE 1: CRÉER LE MUTEX
  // ============================================================
  
  Serial.println("[SETUP] Création du mutex...");
  ledMutex = xSemaphoreCreateMutex();
  
  // Vérification critique: le mutex a-t-il été créé ?
  if (ledMutex == NULL) {
    Serial.println("[ERREUR] Échec de création du mutex!");
    Serial.println("[ERREUR] Programme arrêté.");
    while (1) {
      // Boucle infinie = arrêt du programme
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
  Serial.println("[SETUP] Mutex créé avec succès ✓");
  
  // ============================================================
  // ÉTAPE 2: CRÉER LES TÂCHES
  // ============================================================
  
  Serial.println("[SETUP] Création des tâches...");
  
  // Tâche 1: Clignotement rapide
  BaseType_t result1 = xTaskCreatePinnedToCore(
    taskFastBlink,        // Fonction de la tâche
    "FastBlink",          // Nom (pour débogage)
    STACK_SIZE,           // Taille de la pile
    NULL,                 // Paramètre (aucun)
    TASK_PRIORITY,        // Priorité
    NULL,                 // Handle (pas besoin)
    app_cpu               // Épingler sur Core 1
  );
  
  if (result1 != pdPASS) {
    Serial.println("[ERREUR] Échec création tâche FastBlink!");
  } else {
    Serial.println("[SETUP] Tâche FastBlink créée ✓");
  }
  
  // Tâche 2: Clignotement lent
  BaseType_t result2 = xTaskCreatePinnedToCore(
    taskSlowBlink,        // Fonction de la tâche
    "SlowBlink",          // Nom (pour débogage)
    STACK_SIZE,           // Taille de la pile
    NULL,                 // Paramètre (aucun)
    TASK_PRIORITY,        // Priorité (même que Tâche 1)
    NULL,                 // Handle (pas besoin)
    app_cpu               // Épingler sur Core 1
  );
  
  if (result2 != pdPASS) {
    Serial.println("[ERREUR] Échec création tâche SlowBlink!");
  } else {
    Serial.println("[SETUP] Tâche SlowBlink créée ✓");
  }
  
  // Tâche 3: Monitoring (optionnel - commentez si non nécessaire)
  BaseType_t result3 = xTaskCreatePinnedToCore(
    taskMonitor,          // Fonction de la tâche
    "Monitor",            // Nom
    STACK_SIZE,           // Taille de la pile
    NULL,                 // Paramètre
    TASK_PRIORITY,        // Priorité
    NULL,                 // Handle
    app_cpu               // Core 1
  );
  
  if (result3 != pdPASS) {
    Serial.println("[ERREUR] Échec création tâche Monitor!");
  } else {
    Serial.println("[SETUP] Tâche Monitor créée ✓");
  }
  
  // ============================================================
  // FIN DE L'INITIALISATION
  // ============================================================
  
  Serial.println();
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║  Initialisation terminée avec succès! ║");
  Serial.println("║  Les tâches sont maintenant actives.  ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  
  // Les tâches s'exécutent maintenant automatiquement!
  // setup() se termine mais les tâches continuent à tourner
}

// ============================================================
// FONCTION LOOP: BOUCLE PRINCIPALE
// ============================================================

void loop() {
  // Dans un programme FreeRTOS, loop() est optionnel
  // Les tâches s'exécutent indépendamment
  
  // On peut utiliser loop() pour des opérations non critiques
  // Par exemple: lire des capteurs, gérer des menus, etc.
  
  // Pour ce programme, on laisse loop() vide
  vTaskDelay(pdMS_TO_TICKS(1000));  // Économise le CPU
}
